#include "blockpacketmap.h"
#include "macros.h"         /* NUM_2_STR */
#include <assert.h>         /* assert, _Static_assert */
#include <stdio.h>          /* fprintf, stderr */
#include <stdlib.h>         /* calloc, free */

#define NB_BITS_ITEM (sizeof(*((tBlockPacketMap*) NULL)->_pItems)*8)

bool initMap(tBlockPacketMap* const pBlockPacketMap)
{
    // Compute the number of items needed.
    if(pBlockPacketMap->_header._packetTotal < 1){
        pBlockPacketMap->_header._packetTotal = 0;
        pBlockPacketMap->_header._nbItems = 0;
        pBlockPacketMap->_pItems = NULL;
        return;
    }
    _Static_assert(
        MAX_MAP_NUMBER >= 1,
        "MAX_MAP_NUMBER must be greater than or equal to one."
    );
    if(pBlockPacketMap->_header._packetTotal >
        ((sizeof(tMapItem)*8)*(MAX_MAP_NUMBER - 1) + 1))
    {
        fprintf(
            stderr,
            "Packet total is too high: %u > %lu.\n",
            pBlockPacketMap->_header._packetTotal,
            (sizeof(tMapItem)*8)*(MAX_MAP_NUMBER - 1) + 1
        );
        pBlockPacketMap->_header._packetTotal = 0;
        pBlockPacketMap->_header._nbItems = 0;
        pBlockPacketMap->_pItems = NULL;
        return FALSE;
    }
    pBlockPacketMap->_header._nbItems =
        (pBlockPacketMap->_header._packetTotal - 1) / NB_BITS_ITEM + 1;
    pBlockPacketMap->_pItems =
        calloc(
            pBlockPacketMap->_header._nbItems,
            sizeof(((tBlockPacketMap*) NULL)->_pItems)
        );
    if(pBlockPacketMap->_pItems == NULL){
        fprintf(
            stderr, "Fail to allocate memory at %s line %d.\n",
            __FILE__, __LINE__
        );
        pBlockPacketMap->_header._packetTotal = 0;
        pBlockPacketMap->_header._nbItems = 0;
        return FALSE;
    }
    return TRUE;
}

void setMap(tBlockPacketMap* const pBlockPacketMap,
    const tPacketNumber packetNumber)
{
    if( (pBlockPacketMap != NULL) &&
        (packetNumber < pBlockPacketMap->_header._packetTotal) )
    {
        pBlockPacketMap->_pItems[packetNumber / NB_BITS_ITEM] |=
            (1 << (packetNumber % NB_BITS_ITEM));
    }
}

void clearMap(tBlockPacketMap* const pBlockPacketMap,
    const tPacketNumber packetNumber)
{
    if( (pBlockPacketMap != NULL) &&
        (packetNumber < pBlockPacketMap->_header._packetTotal) )
    {
        pBlockPacketMap->_pItems[packetNumber / NB_BITS_ITEM] &=
            ~(1 << (packetNumber % NB_BITS_ITEM));
    }
}

void toggleMap(tBlockPacketMap* const pBlockPacketMap,
    const tPacketNumber packetNumber)
{
    if( (pBlockPacketMap != NULL) &&
        (packetNumber < pBlockPacketMap->_header._packetTotal) )
    {
        pBlockPacketMap->_pItems[packetNumber / NB_BITS_ITEM] ^=
            (1 << (packetNumber % NB_BITS_ITEM));
    }
}

bool getMap(const tBlockPacketMap* const pBlockPacketMap,
    const tPacketNumber packetNumber)
{
    if( (pBlockPacketMap != NULL) &&
        (packetNumber < pBlockPacketMap->_header._packetTotal) )
    {
        return (
            pBlockPacketMap->_pItems
                [packetNumber / NB_BITS_ITEM] &
                    (1 << (packetNumber % NB_BITS_ITEM))
            ) != 0 ? TRUE : FALSE;
    }
    return FALSE;
}

bool isMapFull(const tBlockPacketMap* const pBlockPacketMap)
{
    if((pBlockPacketMap != NULL) && (pBlockPacketMap->_header._nbItems != 0)){
        if(pBlockPacketMap->_header._nbItems > 1){
            tMapNumber i = 0;
            for(; i < (pBlockPacketMap->_header._nbItems - 1); ++i){
                if(pBlockPacketMap->_pItems[i] != UINT32_MAX){
                    return FALSE;
                }
            }
        }
        // Last map item case.
        const tMapItem lastItem =
            pBlockPacketMap->_pItems[pBlockPacketMap->_header._nbItems - 1];
        if(lastItem != UINT32_MAX){
            const uint32_t mask = (
                    1 << (
                        ((pBlockPacketMap->_header._packetTotal - 1) %
                            NB_BITS_ITEM) + 1
                    )
                ) - 1;
            return (lastItem & mask) == mask ? TRUE : FALSE;
        }
        return TRUE;
    }
    return TRUE;
}

void closeMap(tBlockPacketMap* const pBlockPacketMap)
{
    if((pBlockPacketMap != NULL) && (pBlockPacketMap->_header._nbItems != 0)){
        assert(pBlockPacketMap->_pItems != NULL);
        free(pBlockPacketMap->_pItems);
        pBlockPacketMap->_pItems = NULL;
        pBlockPacketMap->_header._nbItems = 0;
        pBlockPacketMap->_header._packetTotal = 0;
    }
}
