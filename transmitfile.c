#include "transmitfile.h"
#include "server.h"
#include "constantes.h"
#include "macros.h"         /* NUM_2_STR */
#include "types.h"
#include "parsefile.h"      /* tDataBlock */
#include <assert.h>         /* assert */
#include <stdlib.h>         /* EXIT_FAILURE */
#include <string.h>         /* strlen */
#include <stdio.h>          /* fprintf, stderr */

void transmitFile(const char* const outputDir, const char* const localAddr,
    const char* const multAddr, const uint16_t port)
{
    assert(outputDir != NULL);
    // Build index filename.
    char* const indexFilename = buildIndexFileName(outputDir);
    // First read index file if present.
    tIndexTable indexTable;
    if(readIndexFile(indexFilename, &indexTable) == FALSE){
        fprintf(
            stderr,
            "Fail to read index file: %s (not found).\n",
            indexFilename
        );
        free(indexFilename);
        exit(EXIT_FAILURE);
    }
    // Allocate necessary data block table.
    tDataBlock* const pDataBlock =
        malloc(sizeof(tDataBlock)*indexTable._nbItems);
    if(pDataBlock == NULL){
        fprintf(
            stderr,
            "Fail to allocate memory at %s line %d.\n",
            __FILE__, __LINE__
        );
        free(indexFilename);
        exit(EXIT_FAILURE);
    }
    // Then read every block files.
    char* const blockFilename =
        malloc(strlen(outputDir) + (ADD_BLOCK_FILENAME_SIZE) + 1);
    if(blockFilename == NULL){
        fprintf(
            stderr,
            "Fail to allocate memory at %s line %d.\n",
            __FILE__, __LINE__
        );
        free(indexFilename);
        free(pDataBlock);
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
        if(readBlockFile(blockFilename, &(pDataBlock[i]), TRUE) == FALSE){
            fprintf(
                stderr,
                "Fail to read block file: '%s'.\n",
                blockFilename
            );
            free(indexFilename);
            free(pDataBlock);
            free(blockFilename);
            exit(EXIT_FAILURE);
        }
    }
    // Free filenames (no more needed).
    free(blockFilename);
    free(indexFilename);
    // Initialize the server and start sending file blocks.
    {
        tMultServer server;
        initServer(&server, localAddr, multAddr, port);
        runServer(&server, pDataBlock, indexTable._nbItems);
        closeServer(&server);
    }
    // Free index table and data blocks (no more needed).
    free(indexTable._pItems);
    for(i = 0; i < indexTable._nbItems; ++i){
        free(pDataBlock[i]._pPayload);
    }
    free(pDataBlock);
}
