/* 
 * File:   constantes.h
 * Author: pilluh
 *
 * Created on 28 décembre 2015, 14:56
 */

#ifndef CONSTANTES_H
#define CONSTANTES_H

#include <stdint.h>     /* uint8_t */
#include "types.h"      /* tPacketNumber, tBlockNumber, tBlockSize */

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------- Constant definitions -------------------------- */
// Default command line options.
#define PREPARE_OPTION      "fprepare"
#define TRANSMIT_OPTION     "ftransmit"
#define RECEIVE_OPTION      "freceive"
// Default command line parameters.
#define DEF_DATA_DIRECTORY  "/tmp/mltcastdst"
#define DEF_BLOCK_SIZE      ((tBlockSize) 65536)
#define DEF_MULTI_ADDR      "226.1.1.1"
#define DEF_LOCAL_ADDR      "10.0.2.15"
#define DEF_PORT_NUMBER     (4321)
// File naming conventions.
#define INDEX_BASENAME      "data.index"
#define DATA_BASENAME       "data.block"
#define MAP_BASENAME_END    ".map"
#define MAX_BLOCK_DIGITS    5
#define MAX_BLOCK_NUMBER    ((tBlockNumber) 65534)
#define MAX_PACKET_NUMBER   ((tPacketNumber) 65534)
#define MAX_PACKET_SIZE     ((tPacketSize) 65535)
// Constraints constants.
#define MIN_BLOCK_SIZE      ((tBlockSize) 1024)
// Transmit option.
#define BLOCK_SEND_REPEAT   (2)
// Receive option.
#define DISCARD_BLOCK_WITH_NEXT_ONE
// Platform dependant platform.
#ifdef _WIN32
    #define DIRECTORY_SEPARATOR "\\"
#else
    #define DIRECTORY_SEPARATOR "/"
#endif
// General utility constants.
#define false   (FALSE)
#define true    (TRUE)
#define ADD_BLOCK_FILENAME_SIZE (                   \
    (sizeof(DIRECTORY_SEPARATOR) - sizeof(char)) +  \
    (sizeof(DATA_BASENAME) - sizeof(char)) +        \
    (MAX_BLOCK_DIGITS))
#define INVALID_BLOCK_NUMBER ((tBlockNumber) (MAX_BLOCK_NUMBER + 1))
#define ADD_MAP_FILENAME_SIZE (                     \
    (sizeof(DIRECTORY_SEPARATOR) - sizeof(char)) +  \
    (sizeof(DATA_BASENAME) - sizeof(char)) +        \
    (MAX_BLOCK_DIGITS) +                            \
    (sizeof(MAP_BASENAME_END)))

#ifdef __cplusplus
}
#endif

#endif /* CONSTANTES_H */

