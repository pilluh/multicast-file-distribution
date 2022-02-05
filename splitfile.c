#include "splitfile.h"
#include "constantes.h"
#include "macros.h"     /* NUM_2_STR */
#include "crc32.h"      /* crc32c */
#include "parsefile.h"  /* readIndexFile */
#include <stdio.h>      /* fopen, sprintf, fprintf, stderr */
#include <stdlib.h>     /* EXIT_FAILURE, EXIT_SUCCESS */
#include <string.h>     /* strlen, strerror */
#include <assert.h>     /* assert */
#include <sys/stat.h>   /* mkdir */
#include <errno.h>      /* errno, EEXIST */
#include <sys/types.h>
#include <unistd.h>

void createOutputDir(const char* const outputDir)
{
    assert(outputDir != NULL);
    if((mkdir(outputDir, 0777) != 0) && (errno != EEXIST)){
        fprintf(
            stderr,
            "Fail to create directory: %s (%d: %s).\n",
            outputDir, errno, strerror(errno)
        );
        exit(EXIT_FAILURE);
    }
}

void resetOuputDir(const char* const outputDir)
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
        return;
    }
    // Then remove every block files.
    char* const blockFilename =
        malloc(strlen(outputDir) + (ADD_BLOCK_FILENAME_SIZE) + 1);
    if(blockFilename == NULL){
        fprintf(
            stderr,
            "Fail to allocate memory at %s line %d.\n", __FILE__, __LINE__
        );
        exit(EXIT_FAILURE);
    }
    blockFilename[0] = '\0';
    strcat(blockFilename, outputDir);
    strcat(blockFilename, DIRECTORY_SEPARATOR);
    strcat(blockFilename, DATA_BASENAME);
    const size_t blockFileNameIndex = strlen(blockFilename);
    tBlockNumber i = 0;
    for(; i < indexTable._nbItems; ++i){
        sprintf(
            blockFilename + blockFileNameIndex,
            "%0" NUM_2_STR(MAX_BLOCK_DIGITS) "d",
            indexTable._pItems[i]._number
        );
        if(remove(blockFilename) != 0){
            fprintf(
                stderr,
                "Fail to remove block file: '%s' (%d: %s).\n",
                blockFilename, errno, strerror(errno)
            );
            exit(EXIT_FAILURE);
        }
    }
    // Free block filename (no more needed).
    free(blockFilename);
    // Free index table (no more needed).
    free(indexTable._pItems);
    // Lastly remove the index file.
    if(remove(indexFilename) != 0){
        fprintf(
            stderr,
            "Fail to remove index file: '%s' (%d: %s) .\n",
            indexFilename, errno, strerror(errno)
        );
        free(indexFilename);
        exit(EXIT_FAILURE);
    }
    // Free index filename.
    free(indexFilename);
}

void splitFile(const char* const fileName,
               const char* const outputDir,
               const tBlockSize blockSize)
{
    assert((fileName != NULL) && (outputDir != NULL));
    // Check block size parameter.
    if(blockSize < MIN_BLOCK_SIZE){
        fprintf(
            stderr,
            "Invalid block size: %zu < " NUM_2_STR(MIN_BLOCK_SIZE) ".\n",
            blockSize
        );
        exit(EXIT_FAILURE);
    }
    // Create output files directory.
    createOutputDir(outputDir);
    // Reset previous output files.
    resetOuputDir(outputDir);
    // Open input file.
    FILE* const pFile = fopen(fileName, "rb");
    if(pFile == NULL){
        fprintf(stderr, "Fail to open input file: '%s'.\n", fileName);
        exit(EXIT_FAILURE);
    }
    // Get file description.
    struct stat buf;
    stat(fileName, &buf);
    if(buf.st_size < 1){
        fprintf(stderr, "Invalid input file: '%s' (empty).\n", fileName);
        fclose(pFile);
        exit(EXIT_FAILURE);
    }
    // Compute number of items needed.
    tIndexTable indexTable = {
        ((buf.st_size - 1) / blockSize) + 1,
        NULL
    };
    // Check the max number of blocks.
    if(indexTable._nbItems > (MAX_BLOCK_NUMBER + 1)){
        fprintf(
            stderr,
            "Too much blocks will be generated: %u > "
                NUM_2_STR(MAX_BLOCK_NUMBER + 1) ".\n",
            indexTable._nbItems
        );
        fclose(pFile);
        exit(EXIT_FAILURE);
    }
    // Allocate item table.
    indexTable._pItems = malloc(
        indexTable._nbItems*sizeof(*indexTable._pItems)
    );
    if(indexTable._pItems == NULL){
        fprintf(
            stderr,
            "Fail to allocate memory at %s line %d.\n",
            __FILE__, __LINE__
        );
        fclose(pFile);
        exit(EXIT_FAILURE);
    }
    // Allocate data buffer memory.
    unsigned char* const pData = malloc(blockSize);
    if(pData == NULL){
        fprintf(
            stderr,
            "Fail to allocate memory at %s line %d.\n",
            __FILE__, __LINE__
        );
        fclose(pFile);
        exit(EXIT_FAILURE);
    }
    size_t result;
    tBlockNumber blockNumber = 0;
    tBlockOffset blockOffset = 0;
    do{
        result = fread(pData, 1, blockSize, pFile);
        if(result == 0){
            break;
        }
        // Set index item.
        if(blockNumber < indexTable._nbItems){
            tIndexItem* const item = &(indexTable._pItems[blockNumber]);
            item->_number = blockNumber;
            item->_offset = blockOffset;
        }else{
            fprintf(
                stderr,
                "Out of bounds block number: %u >= %u.\n",
                blockNumber,
                indexTable._nbItems
            );
            free(pData);
            fclose(pFile);
            exit(EXIT_FAILURE);
        }
        // Null terminates the data (for crc32 computing).
        const tDataBlock dataBlock = {
            {
                result,
                crc32c(pData, result),
                blockNumber,
                0
            },
            pData
        };
        // Write data block file.
        createBlockFile(outputDir, &dataBlock, FALSE);
        // Increment block offset.
        blockOffset += result;
        // Pass to next block number.
        ++blockNumber;
    }while(result == blockSize);
    // Create index file.
    createIndexFile(outputDir, &indexTable);
    // Free buffer memory (no more needed).
    free(pData);
    // Free index table.
    free(indexTable._pItems);
    // Close the input file.
    if(fclose(pFile) != 0){
        fprintf(stderr, "Fail to close input file: '%s'.\n", fileName);
        exit(EXIT_FAILURE);
    }
}