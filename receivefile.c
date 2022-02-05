#include "receivefile.h"
#include "splitfile.h"      /* createOutputDir, resetOuputDir */
#include "crc32.h"          /* crc32c */
#include "macros.h"         /* NUM_2_STR */
#include "types.h"          /* tChecksum */
#include "constantes.h"     /* INVALID_BLOCK_NUMBER */
#include "client.h"
#include "blockpacketmap.h"
#include "parsefile.h"
#include <stddef.h>         /* NULL */
#include <stdlib.h>         /* EXIT_SUCCESS, malloc, calloc, free */
#include <stdio.h>          /* fprintf, stderr */
#include <assert.h>         /* assert */
#include <string.h>         /* memcpy */
#include <errno.h>          /* errno, EEXIST */

bool allocateIndexTable(tIndexTable* const pIndexTable,
                        const tDataPacket* const pDataPacket)
{
    assert((pIndexTable != NULL) && (pDataPacket != NULL));
    // Allocate the index table.
    if(pDataPacket->_header._blockTotal > MAX_BLOCK_NUMBER){
        fprintf(
            stderr,
            "Number of blocks exceeds maximum authorized: "
                "%u > " NUM_2_STR(MAX_BLOCK_NUMBER) ".\n",
            pDataPacket->_header._blockTotal
        );
        return FALSE;
    }
    // Allocate the index table (set it to zero).
    pIndexTable->_pItems = calloc(
        pDataPacket->_header._blockTotal, sizeof(*pIndexTable->_pItems)
    );
    if(pIndexTable->_pItems == NULL){
        fprintf(
            stderr,
            "Fail to allocate memory at %s line %d.\n",
            __FILE__, __LINE__
        );
        return FALSE;
    }
    // Update index table size.
    pIndexTable->_nbItems = pDataPacket->_header._blockTotal;
    // Initialize index table values.
    const tIndexItem initItem = {INVALID_BLOCK_NUMBER, 0};
    tBlockNumber i = 0;
    for(; i < pIndexTable->_nbItems; ++i){
        pIndexTable->_pItems[i] = initItem;
    }
    return TRUE;
}

bool restoreBlockFromMapFile(const char* const outputDir,
                             const tBlockNumber blockNumber,
                             tBlockPacketMap* const pBlockPacketMap,
                             tDataBlock* const pDataBlock)
{
    // Build map filename.
    char* const mapFileName = buildMapFileName(outputDir, blockNumber);
    if(mapFileName == NULL){
        return FALSE;
    }
    // Read block packet map if available.
    if(readMapFile(mapFileName, pBlockPacketMap) == TRUE){
        // Remove it if successfully read.
        if(remove(mapFileName) != 0){
            fprintf(
                stderr,
                "Fail to remove map file: '%s' (%d: %s).\n",
                mapFileName, errno, strerror(errno)
            );
        }
        // Build block filename.
        char* const blockFileName = buildBlockFileName(outputDir, blockNumber);
        if(blockFileName == NULL){
            // Free block packet map.
            closeMap(pBlockPacketMap);
            // Free map filename.
            free(mapFileName);
            return FALSE;
        }
        // Read block file.
        if(readBlockFile(blockFileName, pDataBlock, FALSE) != TRUE){
            // Free block packet map.
            closeMap(pBlockPacketMap);
        }else{
            // Free map filename.
            free(mapFileName);
            // Free block filename.
            free(blockFileName);
            return TRUE;
        }
        // Free block filename.
        free(blockFileName);
    }
    // Free map filename.
    free(mapFileName);
    return FALSE;
}

void receiveFile(const char* const fileName, const char* const outputDir,
    const char* const localAddr, const char* const multAddr,
    const uint16_t port)
{
    assert((fileName != NULL) && (outputDir != NULL));
    // Create output files directory.
    createOutputDir(outputDir);
    // Reset previous output files.
    resetOuputDir(outputDir);
    // Initialize client.
    const int sd = initClient(localAddr, multAddr, port);
    tDataPacket dataPacket;
    tIndexTable indexTable = {0, NULL};
    tDataBlock dataBlock = {{0, 0, 0, 0}, NULL};
    tBlockNumber nbBlockRead = 0;
    tBlockPacketMap blockPacketMap = {{0, 0}, NULL};
    tPacketSize maxPacketSize = 0;
    for(;;){
        // Read incoming data packet by packet.
        if(readPacket(sd, &dataPacket) != TRUE){
            continue;
        }
        // Allocate the index table on the first received block.
        if(indexTable._pItems == NULL){
            // Try to allocate the index table.
            if(allocateIndexTable(&indexTable, &dataPacket) != TRUE){
                // Ignore the packet.
                goto free_packet;
            }
        }
        // Check the block total number are consistent with the previous one.
        else if(indexTable._nbItems != dataPacket._header._blockTotal){
            fprintf(
                stderr,
                "Inconsistent block total number received: %u != %u.\n",
                indexTable._nbItems,
                dataPacket._header._blockTotal
            );
            // Ignore the packet.
            goto free_packet;
        }
        // Check if the block number is expected.
        if(
            (dataBlock._pPayload != NULL) &&
            (dataPacket._header._blockNumber != dataBlock._header._blockNumber)
        )
        {
#ifdef DISCARD_BLOCK_WITH_NEXT_ONE
            if( (dataPacket._header._blockNumber ==
                    (dataBlock._header._blockNumber + 1)) ||
                (
                    ((dataBlock._header._blockNumber + 1) ==
                        indexTable._nbItems) &&
                    (dataPacket._header._blockNumber == 0) &&
                    (indexTable._nbItems != 1)
                ) )
            {
                // Write the data block (what is available).
                if(createBlockFile(outputDir, &dataBlock, FALSE) != FALSE){
                    // Write the block map file.
                    createMapFile(
                        outputDir, dataBlock._header._blockNumber,
                        &blockPacketMap
                    );
                }
                // Free block packet map.
                closeMap(&blockPacketMap);
                // Reset data block.
                free(dataBlock._pPayload);
                dataBlock._pPayload = NULL;
                // Try to restore previously stored block and map state.
                restoreBlockFromMapFile(
                    outputDir, dataPacket._header._blockNumber,
                    &blockPacketMap, &dataBlock
                );
            }else{
                // Ignore the packet.
                goto free_packet;
            }
#else
            // Ignore the packet.
            goto free_packet;
#endif /* DISCARD_BLOCK_WITH_NEXT_ONE */
        }
        // Get the index table item for this block.
        tIndexItem* const pItem =
            &(indexTable._pItems[dataPacket._header._blockNumber]);
        // Check the block as not already been retrieved.
        if(pItem->_number != INVALID_BLOCK_NUMBER){
            // Ignore the packet.
            goto free_packet;
        }
        // Check the packet as not already been retrieved.
        else if(getMap(&blockPacketMap, dataPacket._header._packetNumber)
            == TRUE)
        {
            // Ignore the packet.
            goto free_packet;
        }
        // Listen the first block received.
        else if(dataBlock._pPayload == NULL){
            // Allocate the block payload only with the not the last packet.
            if(dataPacket._header._packetNumber ==
                (dataPacket._header._packetTotal - 1))
            {
                // Ignore the packet.
                goto free_packet;
            }
            // Try to restore previously stored block and map state.
            if(restoreBlockFromMapFile(
                outputDir, dataPacket._header._blockNumber,
                &blockPacketMap, &dataBlock
            ) != TRUE)
            {
                // Allocate the block payload.
                dataBlock._pPayload = calloc(
                    dataPacket._header._packetTotal,
                    dataPacket._header._payloadSize
                );
                if(dataBlock._pPayload == NULL){
                    fprintf(
                        stderr,
                        "Fail to allocate memory at %s line %d.\n",
                        __FILE__, __LINE__
                    );
                    // Ignore the packet.
                    goto free_packet;
                }
                dataBlock._header._blockNumber =
                    dataPacket._header._blockNumber;
                dataBlock._header._checksum =
                    dataPacket._header._checksum;
                dataBlock._header._payloadSize =
                    dataPacket._header._packetTotal*
                        dataPacket._header._payloadSize;
                // Allocate the block packet map.
                blockPacketMap._header._packetTotal =
                    dataPacket._header._packetTotal;
                initMap(&blockPacketMap);
            }
            // Memorize the max packet size.
            maxPacketSize = dataPacket._header._payloadSize;
        }
        // Check packet total consistency.
        else if(blockPacketMap._header._packetTotal !=
            dataPacket._header._packetTotal)
        {
            fprintf(
                stderr,
                "Inconsistent packet total number received: %u != %u.\n",
                blockPacketMap._header._packetTotal,
                dataPacket._header._packetTotal
            );
            // Ignore the packet.
            goto free_packet;
        }
        // Check checksum consistency.
        else if(dataBlock._header._checksum !=
            dataPacket._header._checksum)
        {
            fprintf(
                stderr,
                "Inconsistent packet checksum received: %u != %u.\n",
                dataBlock._header._checksum,
                dataPacket._header._checksum
            );
            // Ignore the packet.
            goto free_packet;
        }
        // Check block total consistency.
        else if(dataBlock._header._blockNumber !=
            dataPacket._header._blockNumber)
        {
            fprintf(
                stderr,
                "Inconsistent packet block number received: %u != %u.\n",
                dataBlock._header._blockNumber,
                dataPacket._header._blockNumber
            );
            // Ignore the packet.
            goto free_packet;
        }
        // Adjust data block payload if this is the last packet.
        if(dataPacket._header._packetNumber ==
            (dataPacket._header._packetTotal - 1))
        {
            // The last packet size can not be greater than the previous ones.
            if(dataPacket._header._payloadSize > maxPacketSize){
                fprintf(
                    stderr,
                    "Inconsistent packet payload size received: %u != %u.\n",
                    dataPacket._header._payloadSize, maxPacketSize
                );
                // Ignore the packet.
                goto free_packet;
            }
            dataBlock._header._payloadSize -= (
                maxPacketSize - dataPacket._header._payloadSize
            );
        }
        // Check the block will not overflow.
        const tBlockSize blockOffset =
            maxPacketSize*dataPacket._header._packetNumber;
        if((blockOffset + dataPacket._header._payloadSize) >
            dataBlock._header._payloadSize)
        {
            fprintf(
                stderr,
                "Invalid payload size received: %zu + %u > %zu.\n",
                dataBlock._header._payloadSize,
                dataPacket._header._payloadSize,
                dataBlock._header._payloadSize
            );
            // Ignore the packet.
            goto free_packet;
        }
        // Append the packet at the end of the block.
        memcpy(
            dataBlock._pPayload + blockOffset,
            dataPacket._pPayload,
            dataPacket._header._payloadSize
        );
        // Update the next packet number.
        setMap(&blockPacketMap, dataPacket._header._packetNumber);
        if(isMapFull(&blockPacketMap) == TRUE){
            // Check data block checksum.
            const tChecksum checksum =
                crc32c(dataBlock._pPayload, dataBlock._header._payloadSize);
            if(checksum == dataBlock._header._checksum){
                // If the last packet was received, write the block.
                createBlockFile(outputDir, &dataBlock, FALSE);
                // Update the index table (marked it as completed).
                pItem->_offset = dataBlock._header._payloadSize;
                pItem->_number = dataBlock._header._blockNumber;
            }else{
                fprintf(
                    stderr,
                    "Invalid data block checksum detected: number %u.\n",
                    dataBlock._header._blockNumber
                );
            }
            // Reset data block.
            free(dataBlock._pPayload);
            dataBlock._pPayload = NULL;
            // Reset block packet map.
            closeMap(&blockPacketMap);
            // Check if the last block has been read (and it was correct).
            if( (checksum == dataBlock._header._checksum) &&
                (++nbBlockRead == indexTable._nbItems) )
            {
                // Free the allocated packet memory.
                free(dataPacket._pPayload);
                break;
            }
        }
free_packet:
        // Free the allocated packet memory.
        free(dataPacket._pPayload);
    }
    // Terminate client.
    closeClient(sd);
    // Reorganize index table by computing offset by block.
    tBlockOffset offset = 0;
    tBlockNumber i = 0;
    for(; i < indexTable._nbItems; ++i){
        tIndexItem* const pItem = &(indexTable._pItems[i]);
        tBlockOffset blockOffset = pItem->_offset;
        pItem->_offset = offset;
        offset += blockOffset;
    }
    // Create the index file.
    createIndexFile(outputDir, &indexTable);
    // Free the index table.
    if(indexTable._pItems != NULL){
        free(indexTable._pItems);
    }
    // Generate the input file from block files.
    generateDataFile(fileName, outputDir);
    // Reset previous output files.
    resetOuputDir(outputDir);
}
