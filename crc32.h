/* 
 * File:   crc32.h
 * Author: pilluh
 *
 * Created on 28 décembre 2015, 16:02
 */

#ifndef CRC32_H
#define CRC32_H

#include "types.h"      /* tChecksum */
#include <stddef.h>     /* size_t */

#ifdef __cplusplus
extern "C" {
#endif

tChecksum crc32a(const unsigned char* const message);
tChecksum crc32b(const unsigned char* const message);
tChecksum crc32c(const unsigned char* const message, const size_t size);
tChecksum crc32cx(const unsigned char* message);

#ifdef __cplusplus
}
#endif

#endif /* CRC32_H */

