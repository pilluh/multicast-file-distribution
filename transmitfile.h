/* 
 * File:   transmitfile.h
 * Author: pilluh
 *
 * Created on 28 décembre 2015, 15:06
 */

#ifndef TRANSMITFILE_H
#define TRANSMITFILE_H

#include <stdint.h>     /* uint16_t */

#ifdef __cplusplus
extern "C" {
#endif

void transmitFile(const char* const outputDir, const char* const localAddr,
    const char* const multAddr, const uint16_t port);

#ifdef __cplusplus
}
#endif

#endif /* TRANSMITFILE_H */

