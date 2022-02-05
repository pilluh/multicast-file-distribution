/* 
 * File:   types.h
 * Author: pilluh
 *
 * Created on 28 décembre 2015, 15:31
 */

#ifndef TYPES_H
#define TYPES_H

#include <stddef.h>     /* size_t */
#include <stdint.h>     /* uint16_t, uint32_t */

#ifdef __cplusplus
extern "C" {
#endif

typedef enum bool{
    FALSE   = 0,
    TRUE    = 1
} bool;

typedef uint16_t    tBlockNumber;
typedef uint32_t    tBlockOffset;
typedef size_t      tBlockSize;
typedef uint16_t    tPacketNumber;
typedef uint16_t    tPacketSize;
typedef uint32_t    tChecksum;

typedef struct sIndexItem{
    tBlockNumber    _number;
    tBlockOffset    _offset;
} tIndexItem;

typedef struct sIndexTable{
    tBlockNumber    _nbItems;
    tIndexItem*     _pItems;
} tIndexTable;

typedef struct sDataBlockHeader{
    tBlockSize      _payloadSize;
    tChecksum       _checksum;
    tBlockNumber    _blockNumber;
    uint16_t        _padding;
} tDataBlockHeader;

typedef struct sDataBlock{
    tDataBlockHeader    _header;
    void*               _pPayload;
} tDataBlock;

typedef struct sDataPacketHeader{
    tChecksum       _checksum;
    tBlockNumber    _blockNumber;
    tBlockNumber    _blockTotal;
    tPacketNumber   _packetNumber;
    tPacketNumber   _packetTotal;
    tPacketSize     _payloadSize;
    uint16_t        _padding;
} tDataPacketHeader;

typedef struct sDataPacket{
    tDataPacketHeader   _header;
    void*               _pPayload;
} tDataPacket;

#ifdef __cplusplus
}
#endif

#endif /* TYPES_H */

