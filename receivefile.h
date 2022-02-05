/* 
 * File:   receivefile.h
 * Author: pilluh
 *
 * Created on 30 décembre 2015, 21:36
 */

#ifndef RECEIVEFILE_H
#define RECEIVEFILE_H

#include <stdint.h>     /* uint16_t */

#ifdef __cplusplus
extern "C" {
#endif

void receiveFile(const char* const fileName, const char* const outputDir,
    const char* const localAddr, const char* const multAddr,
    const uint16_t port);

#ifdef __cplusplus
}
#endif

#endif /* RECEIVEFILE_H */

