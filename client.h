/* 
 * File:   client.h
 * Author: pilluh
 *
 * Created on 31 décembre 2015, 10:39
 */

#ifndef CLIENT_H
#define CLIENT_H

#include "types.h"  /* tDataPacket, bool */
#include <stdint.h> /* uint16_t */

#ifdef __cplusplus
extern "C" {
#endif

int initClient(const char* const localAddr,
    const char* const multAddr, const uint16_t port);
void runClient(const int sd);
bool readPacket(const int sd, tDataPacket* const pDataPacket);
void closeClient(const int sd);

#ifdef __cplusplus
}
#endif

#endif /* CLIENT_H */

