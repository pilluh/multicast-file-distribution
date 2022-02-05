/* 
 * File:   server.h
 * Author: pilluh
 *
 * Created on 31 décembre 2015, 15:49
 */

#ifndef SERVER_H
#define SERVER_H

#include "types.h"      /* tDataBlock, tDataPacket, tBlockNumber */
#include <stdint.h>     /* uint16_t */
#include <netinet/in.h> /* sockaddr_in */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sMultServer{
    int                 _sd;
    struct sockaddr_in  _groupSock;
} tMultServer;

void initServer(tMultServer* const server, const char* const localAddr,
    const char* const multAddr, const uint16_t port);
void runServer(tMultServer* const server,
               const tDataBlock* const pDataBlock,
               const tBlockNumber nDataBlock);
int writePacket(tMultServer* const server, tDataPacket* const pDataPacket);
void closeServer(tMultServer* const server);

#ifdef __cplusplus
}
#endif

#endif /* SERVER_H */

