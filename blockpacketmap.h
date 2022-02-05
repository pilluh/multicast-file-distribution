/* 
 * File:   blockpacketmap.h
 * Author: harald
 *
 * Created on January 9, 2016, 10:43 PM
 */

#ifndef BLOCKPACKETMAP_H
#define	BLOCKPACKETMAP_H

#include "types.h"      /* tPacketNumber */
#include <stdint.h>     /* uint32_t */

#ifdef	__cplusplus
extern "C" {
#endif

typedef uint16_t tMapNumber;
typedef uint32_t tMapItem;

#define MAX_MAP_NUMBER ((tMapNumber) 65535)

typedef struct sBlockPacketMapHeader{
    tPacketNumber _packetTotal;
    tMapNumber    _nbItems;
} tBlockPacketMapHeader;

typedef struct sBlockPacketMap{
    tBlockPacketMapHeader   _header;
    tMapItem*               _pItems;
} tBlockPacketMap;

bool initMap(tBlockPacketMap* const pBlockPacketMap);
void setMap(tBlockPacketMap* const pBlockPacketMap,
    const tPacketNumber packetNumber);
void clearMap(tBlockPacketMap* const pBlockPacketMap,
    const tPacketNumber packetNumber);
void toggleMap(tBlockPacketMap* const pBlockPacketMap,
    const tPacketNumber packetNumber);
bool getMap(const tBlockPacketMap* const pBlockPacketMap,
    const tPacketNumber packetNumber);
bool isMapFull(const tBlockPacketMap* const pBlockPacketMap);
void closeMap(tBlockPacketMap* const pBlockPacketMap);

#ifdef	__cplusplus
}
#endif

#endif	/* BLOCKPACKETMAP_H */

