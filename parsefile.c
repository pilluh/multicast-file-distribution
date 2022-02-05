#include "parsefile.h"
#include "constantes.h" /* INDEX_BASENAME, DIRECTORY_SEPARATOR */
#include "macros.h"     /* NUM_2_STR */
#include <stdio.h>      /* fopen, fprintf, stderr, fgetc, EOF */
#include <stdlib.h>     /* malloc, free */
#include <assert.h>     /* assert */
#include <string.h>     /* strlen */
#include <errno.h>      /* errno, ENOENT */

char* buildIndexFileName(const char* const outputDir)
{
    assert(outputDir != NULL);
    char* const indexFilename =
        malloc(strlen(outputDir) + 2 + strlen(INDEX_BASENAME));
    if(indexFilename == NULL){
        fprintf(
            stderr,
            "Fail to allocate memory at %s line %d.\n",
            __FILE__, __LINE__
        );
        return NULL;
    }
    indexFilename[0] = '\0';
    strcat(indexFilename, outputDir);
    strcat(indexFilename, DIRECTORY_SEPARATOR);
    strcat(indexFilename, INDEX_BASENAME);
    return indexFilename;
}

bool readIndexFile(const char* const fileName, tIndexTable* pIndexTable)
{
    assert((fileName != NULL) && (pIndexTable != NULL));
    // Open index file.
    FILE* const pFile = fopen(fileName, "rb");
    if(pFile == NULL){
        if(errno != ENOENT){
            fprintf(
                stderr,
                "Fail to read index file: %s (%d: %s).\n",
                fileName, errno, strerror(errno)
            );
        }
        return FALSE;
    }
    // First read the number of items.
    size_t result = fread(
        &pIndexTable->_nbItems, sizeof(pIndexTable->_nbItems), 1, pFile
    );
    if(result != 1){
        fprintf(
            stderr,
            "Fail to read index file: '%s' (incorrect item number).\n",
            fileName
        );
        fclose(pFile);
        return FALSE;
    }
    // Allocate the necessary memory storage.
    pIndexTable->_pItems = (tIndexItem*) malloc(
        pIndexTable->_nbItems*sizeof(*pIndexTable->_pItems)
    );
    if(pIndexTable->_pItems == NULL){
        fprintf(
            stderr,
            "Fail to allocate memory at %s line %d.\n",
            __FILE__, __LINE__
        );
        fclose(pFile);
        return FALSE;
    }
    // Then, read the data array.
    result = fread(
        pIndexTable->_pItems, sizeof(*pIndexTable->_pItems),
        pIndexTable->_nbItems, pFile
    );
    if(result != pIndexTable->_nbItems){
        fprintf(
            stderr,
            "Fail to read index file: '%s' (incorrect item array).\n",
            fileName
        );
        fclose(pFile);
        return FALSE;
    }
    // Lastly, close the index file.
    if(fclose(pFile) != 0){
        fprintf(stderr, "Fail to close input file: '%s'.\n", fileName);
        return FALSE;
    }
    return TRUE;
}

bool createIndexFile(const char* const outputDir,
                     const tIndexTable* const pIndexTable)
{
    assert((outputDir != NULL) && (pIndexTable != NULL));
    // Build index filename.
    char* const indexFilename = buildIndexFileName(outputDir);
    if(indexFilename == NULL){
        return FALSE;
    }
    // Open index file.
    FILE* const pFile = fopen(indexFilename, "wb+");
    if(pFile == NULL){
        fprintf(stderr, "Fail to open index file: '%s'.\n", indexFilename);
        free(indexFilename);
        return FALSE;
    }
    // First write number of items.
    size_t result = fwrite(
        &pIndexTable->_nbItems, sizeof(pIndexTable->_nbItems), 1, pFile
    );
    if(result != 1){
        fprintf(
            stderr,
            "Fail to write into index file: '%s' (index number).\n",
            indexFilename
        );
        free(indexFilename);
        fclose(pFile);
        return FALSE;
    }
    // Then write all blocks.
    result = fwrite(
        pIndexTable->_pItems, sizeof(*pIndexTable->_pItems),
        pIndexTable->_nbItems, pFile
    );
    if(result != pIndexTable->_nbItems){
        fprintf(
            stderr,
            "Fail to write into index file: '%s' (index number).\n",
            indexFilename
        );
        free(indexFilename);
        fclose(pFile);
        return FALSE;
    }
    // Close the index file.
    if(fclose(pFile) != 0){
        fprintf(stderr, "Fail to close index file: '%s'.\n", indexFilename);
        free(indexFilename);
        return FALSE;
    }
    // Free index filename (no more needed).
    free(indexFilename);
    return TRUE;
}

char* buildBlockFileName(const char* const outputDir,
    const tBlockNumber blockNumber)
{
    assert(outputDir != NULL);
    // Check block number.
    if(blockNumber > MAX_BLOCK_NUMBER){
        fprintf(stderr, "Max block number exceeded: %u.\n", blockNumber);
        return NULL;
    }
    // Allocate map filename.
    char* const blockFilename =
        malloc(strlen(outputDir) + (ADD_BLOCK_FILENAME_SIZE) + 1);
    if(blockFilename == NULL){
        fprintf(
            stderr,
            "Fail to allocate memory at %s line %d.\n",
            __FILE__, __LINE__
        );
        return NULL;
    }
    // Complete map filename.
    blockFilename[0] = '\0';
    strcat(blockFilename, outputDir);
    strcat(blockFilename, DIRECTORY_SEPARATOR);
    strcat(blockFilename, DATA_BASENAME);
    sprintf(
        blockFilename + strlen(blockFilename),
        "%0" NUM_2_STR(MAX_BLOCK_DIGITS) "d", blockNumber
    );
    return blockFilename;
}

bool readBlockFile(const char* const fileName,
                   tDataBlock* const pDataBlock,
                   const bool checkChecksum)
{
    assert((fileName != NULL) && (pDataBlock != NULL));
    // Open block file.
    FILE* const pFile = fopen(fileName, "rb");
    if(pFile == NULL){
        fprintf(
            stderr,
            "Fail to read block file: %s (%d: %s).\n",
            fileName, errno, strerror(errno)
        );
        return FALSE;
    }
    // First read the block header.
    size_t result = fread(
        &pDataBlock->_header, sizeof(pDataBlock->_header), 1, pFile
    );
    if(result != 1){
        fprintf(
            stderr,
            "Fail to read block file: '%s' (incorrect header).\n",
            fileName
        );
        fclose(pFile);
        return FALSE;
    }
    // Allocate the payload.
    pDataBlock->_pPayload = malloc(pDataBlock->_header._payloadSize);
    if(pDataBlock->_pPayload == NULL){
        fprintf(
            stderr,
            "Fail to allocate memory at %s line %d.\n",
            __FILE__, __LINE__
        );
        fclose(pFile);
        return FALSE;
    }
    // Then, read the payload.
    result = fread(
        pDataBlock->_pPayload, pDataBlock->_header._payloadSize,
        1, pFile
    );
    if(result != 1){
        fprintf(
            stderr,
            "Fail to read block file: '%s' (incorrect payload).\n",
            fileName
        );
        free(pDataBlock->_pPayload);
        fclose(pFile);
        return FALSE;
    }
    // Check the checksum.
    if( (checkChecksum == TRUE) &&
        (crc32c(pDataBlock->_pPayload, pDataBlock->_header._payloadSize) !=
            pDataBlock->_header._checksum) )
    {
        fprintf(
            stderr,
            "Fail to read block file: '%s' (invalid checksum).\n",
            fileName
        );
        free(pDataBlock->_pPayload);
        fclose(pFile);
        return FALSE;
    }
    // Check the end of file is reached (no ignored data should be there).
    if(fgetc(pFile) != EOF){
        fprintf(
            stderr,
            "Fail to read block file: '%s' (remaining data).\n",
            fileName
        );
        free(pDataBlock->_pPayload);
        fclose(pFile);
        return FALSE;
    }
    // Lastly, close the block file.
    if(fclose(pFile) != 0){
        fprintf(stderr, "Fail to close input file: '%s'.\n", fileName);
        free(pDataBlock->_pPayload);
        return FALSE;
    }
    return TRUE;
}

bool createBlockFile(const char* const outputDir,
                     const tDataBlock* const pDataBlock,
                     const bool checkChecksum)
{
    assert((outputDir != NULL) && (pDataBlock != NULL));
    // Check block number.
    if(pDataBlock->_header._blockNumber > MAX_BLOCK_NUMBER){
        fprintf(
            stderr,
            "Max block number exceeded: %u.\n",
            pDataBlock->_header._blockNumber
        );
        return FALSE;
    }
    // Check the checksum.
    if( (checkChecksum == TRUE) &&
        (crc32c(pDataBlock->_pPayload, pDataBlock->_header._payloadSize) !=
            pDataBlock->_header._checksum) )
    {
        fprintf(
            stderr,
            "Fail to write block file: '%u' (invalid checksum).\n",
            pDataBlock->_header._blockNumber
        );
        return FALSE;
    }
    // Build block filename.
    char* const blockFilename = buildBlockFileName(
        outputDir, pDataBlock->_header._blockNumber
    );
    if(blockFilename == NULL){
        return FALSE;
    }
    // Open block file.
    FILE* const pFile = fopen(blockFilename, "wb+");
    if(pFile == NULL){
        fprintf(stderr, "Fail to open block file: '%s'.\n", blockFilename);
        free(blockFilename);
        return FALSE;
    }
    // First write block header.
    size_t result = fwrite(
        &pDataBlock->_header, sizeof(pDataBlock->_header), 1, pFile
    );
    if(result != 1){
        fprintf(
            stderr,
            "Fail to write into block file: '%s' (header).\n",
            blockFilename
        );
        free(blockFilename);
        fclose(pFile);
        return FALSE;
    }
    // Then write block data.
    result = fwrite(
        pDataBlock->_pPayload, sizeof(*pDataBlock->_pPayload),
        pDataBlock->_header._payloadSize, pFile
    );
    if(result != pDataBlock->_header._payloadSize){
        fprintf(
            stderr,
            "Fail to write into block file: '%s' (payload).\n",
            blockFilename
        );
        free(blockFilename);
        fclose(pFile);
        return FALSE;
    }
    // Close the index file.
    if(fclose(pFile) != 0){
        fprintf(stderr, "Fail to close block file: '%s'.\n", blockFilename);
        free(blockFilename);
        return FALSE;
    }
    // Free block filename (no more needed).
    free(blockFilename);
    return TRUE;
}

char* buildMapFileName(const char* const outputDir,
    const tBlockNumber blockNumber)
{
    assert(outputDir != NULL);
    // Check block number.
    if(blockNumber > MAX_BLOCK_NUMBER){
        fprintf(stderr, "Max block number exceeded: %u.\n", blockNumber);
        return NULL;
    }
    // Allocate map filename.
    char* const mapFileName =
        malloc(strlen(outputDir) + (ADD_MAP_FILENAME_SIZE) + 1);
    if(mapFileName == NULL){
        fprintf(
            stderr,
            "Fail to allocate memory at %s line %d.\n",
            __FILE__, __LINE__
        );
        return FALSE;
    }
    // Complete map filename.
    mapFileName[0] = '\0';
    strcat(mapFileName, outputDir);
    strcat(mapFileName, DIRECTORY_SEPARATOR);
    strcat(mapFileName, DATA_BASENAME);
    sprintf(
        mapFileName + strlen(mapFileName),
        "%0" NUM_2_STR(MAX_BLOCK_DIGITS) "d", blockNumber
    );
    strcat(mapFileName, MAP_BASENAME_END);
    return mapFileName;
}

bool readMapFile(const char* const fileName,
    tBlockPacketMap* const pBlockPacketMap)
{
    assert((fileName != NULL) && (pBlockPacketMap != NULL));
    // Open block file.
    FILE* const pFile = fopen(fileName, "rb");
    if(pFile == NULL){
        if(errno != ENOENT){
            fprintf(
                stderr,
                "Fail to read map file: %s (%d: %s).\n",
                fileName, errno, strerror(errno)
            );
        }
        return FALSE;
    }
    // First read the block header.
    size_t result = fread(
        &pBlockPacketMap->_header, sizeof(pBlockPacketMap->_header), 1, pFile
    );
    if(result != 1){
        fprintf(
            stderr,
            "Fail to read map file: '%s' (incorrect header).\n",
            fileName
        );
        fclose(pFile);
        return FALSE;
    }
    // Allocate the item table.
    pBlockPacketMap->_pItems = malloc(
        pBlockPacketMap->_header._nbItems * sizeof(*pBlockPacketMap->_pItems)
    );
    if(pBlockPacketMap->_pItems == NULL){
        fprintf(
            stderr,
            "Fail to allocate memory at %s line %d.\n",
            __FILE__, __LINE__
        );
        fclose(pFile);
        return FALSE;
    }
    // Then, read the payload.
    result = fread(
        pBlockPacketMap->_pItems, sizeof(*pBlockPacketMap->_pItems),
        pBlockPacketMap->_header._nbItems, pFile
    );
    if(result != pBlockPacketMap->_header._nbItems){
        fprintf(
            stderr,
            "Fail to read map file: '%s' (incorrect data).\n",
            fileName
        );
        fclose(pFile);
        return FALSE;
    }
    // Check the end of file is reached (no ignored data should be there).
    if(fgetc(pFile) != EOF){
        fprintf(
            stderr,
            "Fail to read map file: '%s' (remaining data).\n",
            fileName
        );
        fclose(pFile);
        return FALSE;
    }
    // Lastly, close the block file.
    if(fclose(pFile) != 0){
        fprintf(stderr, "Fail to close map file: '%s'.\n", fileName);
        return FALSE;
    }
    return TRUE;
}

bool createMapFile(const char* const outputDir, const tBlockNumber blockNumber,
    tBlockPacketMap* const pBlockPacketMap)
{
    assert((outputDir != NULL) && (pBlockPacketMap != NULL));
    // Check block number.
    if(blockNumber > MAX_BLOCK_NUMBER){
        fprintf(stderr, "Max block number exceeded: %u.\n", blockNumber);
        return FALSE;
    }
    // Check map number.
    if(pBlockPacketMap->_header._nbItems > MAX_MAP_NUMBER){
        fprintf(stderr,
            "Max map item number exceeded: %u.\n",
            pBlockPacketMap->_header._nbItems
        );
        return FALSE;
    }
    // Build map filename.
    char* const mapFileName =
        malloc(strlen(outputDir) + (ADD_MAP_FILENAME_SIZE) + 1);
    if(mapFileName == NULL){
        fprintf(
            stderr,
            "Fail to allocate memory at %s line %d.\n",
            __FILE__, __LINE__
        );
        return FALSE;
    }
    mapFileName[0] = '\0';
    strcat(mapFileName, outputDir);
    strcat(mapFileName, DIRECTORY_SEPARATOR);
    strcat(mapFileName, DATA_BASENAME);
    sprintf(
        mapFileName + strlen(mapFileName),
        "%0" NUM_2_STR(MAX_BLOCK_DIGITS) "d", blockNumber
    );
    strcat(mapFileName, MAP_BASENAME_END);
    // Open/create map file.
    FILE* const pFile = fopen(mapFileName, "wb+");
    if(pFile == NULL){
        fprintf(stderr, "Fail to open/create map file: '%s'.\n", mapFileName);
        free(mapFileName);
        return FALSE;
    }
    // First write block header.
    size_t result = fwrite(
        &pBlockPacketMap->_header, sizeof(pBlockPacketMap->_header), 1, pFile
    );
    if(result != 1){
        fprintf(
            stderr,
            "Fail to write into map file: '%s' (header).\n",
            mapFileName
        );
        free(mapFileName);
        fclose(pFile);
        return FALSE;
    }
    // Then write block data.
    result = fwrite(
        pBlockPacketMap->_pItems, sizeof(*pBlockPacketMap->_pItems),
        pBlockPacketMap->_header._nbItems, pFile
    );
    if(result != pBlockPacketMap->_header._nbItems){
        fprintf(
            stderr,
            "Fail to write into map file: '%s' (data).\n",
            mapFileName
        );
        free(mapFileName);
        fclose(pFile);
        return FALSE;
    }
    // Close the map file.
    if(fclose(pFile) != 0){
        fprintf(stderr, "Fail to close map file: '%s'.\n", mapFileName);
        free(mapFileName);
        return FALSE;
    }
    // Free block filename (no more needed).
    free(mapFileName);
    return TRUE;
}

bool generateDataFile(const char* const fileName, const char* const outputDir)
{
    assert(outputDir != NULL);
    // Build index filename.
    char* const indexFilename = buildIndexFileName(outputDir);
    // First read index file if present.
    tIndexTable indexTable;
    if(readIndexFile(indexFilename, &indexTable) == FALSE){
        // Free index filename.
        free(indexFilename);
        // The index file does not exist, so nothing to be done.
        return FALSE;
    }
    // Free index filename (no more needed).
    free(indexFilename);
    // Then open output file.
    FILE* const pFile = fopen(fileName, "wb+");
    if(pFile == NULL){
        fprintf(stderr, "Fail to open output file: '%s'.\n", fileName);
        // Free index table.
        free(indexTable._pItems);
        return FALSE;
    }
    // Then read every block files.
    char* const blockFilename =
        malloc(strlen(outputDir) + (ADD_BLOCK_FILENAME_SIZE) + 1);
    if(blockFilename == NULL){
        fprintf(
            stderr,
            "Fail to allocate memory at %s line %d.\n", __FILE__, __LINE__
        );
        // Free index table.
        free(indexTable._pItems);
        return FALSE;
    }
    blockFilename[0] = '\0';
    strcat(blockFilename, outputDir);
    strcat(blockFilename, DIRECTORY_SEPARATOR);
    strcat(blockFilename, DATA_BASENAME);
    tDataBlock dataBlock = {{0, 0, 0, 0}, NULL};
    const size_t blockFileNameIndex = strlen(blockFilename);
    tBlockNumber i = 0;
    for(; i < indexTable._nbItems; ++i){
        sprintf(
            blockFilename + blockFileNameIndex,
            "%0" NUM_2_STR(MAX_BLOCK_DIGITS) "d",
            indexTable._pItems[i]._number
        );
        readBlockFile(blockFilename, &dataBlock, TRUE);
        // Append data block to file.
        if(fwrite(dataBlock._pPayload,
            dataBlock._header._payloadSize, 1, pFile) != 1)
        {
            fprintf(
                stderr,
                "Error appending to output file: '%s'.\n",
                fileName
            );
            // Free data block.
            free(dataBlock._pPayload);
            break;
        }
        // Free data block.
        free(dataBlock._pPayload);
    }
    // Free block filename.
    free(blockFilename);
    // Free index table.
    free(indexTable._pItems);
    return TRUE;
}