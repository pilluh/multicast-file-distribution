/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   parsefile.h
 * Author: pilluh
 *
 * Created on 28 décembre 2015, 16:18
 */

#ifndef PARSEFILE_H
#define PARSEFILE_H

#include "types.h"          /* bool, tIndexTable, pDataBlock */
#include "blockpacketmap.h" /*  */

#ifdef __cplusplus
extern "C" {
#endif

char* buildIndexFileName(const char* const outputDir);
bool readIndexFile(const char* const fileName, tIndexTable* pIndexTable);
bool createIndexFile(const char* const outputDir,
                     const tIndexTable* const pIndexTable);
char* buildBlockFileName(const char* const outputDir,
    const tBlockNumber blockNumber);
bool readBlockFile(const char* const fileName, tDataBlock* const pDataBlock,
                   const bool checkChecksum);
bool createBlockFile(const char* const outputDir,
                     const tDataBlock* const pDataBlock,
                     const bool checkChecksum);
char* buildMapFileName(const char* const outputDir,
    const tBlockNumber blockNumber);
bool readMapFile(const char* const fileName,
    tBlockPacketMap* const pBlockPacketMap);
bool createMapFile(const char* const outputDir, const tBlockNumber blockNumber,
    tBlockPacketMap* const pBlockPacketMap);
bool generateDataFile(const char* const fileName, const char* const outputDir);

#ifdef __cplusplus
}
#endif

#endif /* PARSEFILE_H */

