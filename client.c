#include "client.h"
#include "constantes.h"     /* MAX_BLOCK_NUMBER */
#include "macros.h"         /* NUM_2_STR */
#include <stdlib.h>         /* EXIT_FAILURE */
#include <stdio.h>          /* perror, fprintf, stderr */
#include <string.h>         /* memset */
#include <assert.h>         /* assert */

// Socket includes
#include <sys/types.h>
#include <sys/socket.h>     /* socket */
#include <arpa/inet.h>
#include <netinet/in.h>

int initClient(const char* const localAddr,
    const char* const multAddr, const uint16_t port)
{
    /* Create a datagram socket on which to receive. */
    const int sd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sd < 0){
        perror("Error opening datagram socket (client)");
        exit(EXIT_FAILURE);
    }
    /* Enable SO_REUSEADDR to allow multiple instances of this */
    /* application to receive copies of the multicast datagrams. */
    const int reuse = 1;
    if(setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0){
        perror("Error setting SO_REUSEADDR");
        close(sd);
        exit(EXIT_FAILURE);
    }
    /* Bind to the proper port number with the IP address */
    /* specified as INADDR_ANY. */
    struct sockaddr_in localSock;
    memset(&localSock, 0, sizeof(localSock));
    localSock.sin_family = AF_INET;
    localSock.sin_port = htons(port);
    localSock.sin_addr.s_addr = INADDR_ANY;
    if(bind(sd, (struct sockaddr*) &localSock, sizeof(localSock)) != 0){
        perror("Error binding datagram socket");
        close(sd);
        exit(EXIT_FAILURE);
    }
    /* Join the multicast group 226.1.1.1 on the local 203.106.93.94 */
    /* interface. Note that this IP_ADD_MEMBERSHIP option must be */
    /* called for each local interface over which the multicast */
    /* datagrams are to be received. */
    struct ip_mreq group;
    group.imr_multiaddr.s_addr = inet_addr(multAddr);
    group.imr_interface.s_addr = inet_addr(localAddr);
    if(setsockopt(sd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &group, sizeof(group))
        != 0)
    {
        perror("Error adding multicast group");
        close(sd);
        exit(EXIT_FAILURE);
    }
    return sd;
}

void runClient(const int sd)
{
    char databuf[1024];
    size_t result;
    for(;;){
        /* Read from the socket. */
        result = read(sd, databuf, sizeof(databuf));
        if(result < 0){
            perror("Error reading datagram message");
            close(sd);
            exit(EXIT_FAILURE);
        }
    }
}

bool readPacket(const int sd, tDataPacket* const pDataPacket)
{
    assert(pDataPacket != NULL);
    /* Read header from socket. */
    size_t result = recv(
        sd, &(pDataPacket->_header), sizeof(pDataPacket->_header), MSG_PEEK
    );
    if(result != sizeof(pDataPacket->_header)){
        fprintf(
            stderr,
            "Error reading packet message (invalid header).\n"
        );
        return FALSE;
    }
    // Allocate memory buffer.
    const tPacketSize packetSize =
        sizeof(pDataPacket->_header) + pDataPacket->_header._payloadSize;
    void* const buffer = malloc(packetSize);
    if(buffer == NULL){
        fprintf(
            stderr,
            "Fail to allocate memory at %s line %d.\n",
            __FILE__, __LINE__
        );
        return FALSE;
    }
    /* Read header and payload from the socket. */
    result = recv(sd, buffer, packetSize, MSG_WAITALL);
    if(result != packetSize){
        fprintf(
            stderr,
            "Error reading packet message (invalid packet size).\n"
        );
        free(buffer);
        return FALSE;
    }
    // Copy header.
    memcpy(&(pDataPacket->_header), buffer, sizeof(pDataPacket->_header));
    // Copy payload (translate buffer memory).
    memmove(buffer, buffer + sizeof(pDataPacket->_header),
        pDataPacket->_header._payloadSize);
    pDataPacket->_pPayload = buffer;
    // Check the total block coherency.
    if(pDataPacket->_header._blockTotal > (MAX_BLOCK_NUMBER + 1)){
        fprintf(
            stderr,
            "Number of blocks exceeds maximum authorized: "
                "%u > " NUM_2_STR(MAX_BLOCK_NUMBER) ".\n",
            pDataPacket->_header._blockTotal
        );
        free(buffer);
        return FALSE;
    }
    // Check the block number coherency.
    if(pDataPacket->_header._blockNumber >= pDataPacket->_header._blockTotal){
        fprintf(
            stderr,
            "Invalid block number: %u >= %u.\n",
            pDataPacket->_header._blockNumber,
            pDataPacket->_header._blockTotal
        );
        free(buffer);
        return FALSE;
    }
    // Check the total packet coherency.
    if(pDataPacket->_header._packetTotal > (MAX_PACKET_NUMBER + 1)){
        fprintf(
            stderr,
            "Number of packets exceeds maximum authorized: "
                "%u > " NUM_2_STR(MAX_PACKET_NUMBER) ".\n",
            pDataPacket->_header._packetTotal
        );
        free(buffer);
        return FALSE;
    }
    // Check the block number coherency.
    if(pDataPacket->_header._packetNumber >= pDataPacket->_header._packetTotal){
        fprintf(
            stderr,
            "Invalid packet number: %u >= %u.\n",
            pDataPacket->_header._packetNumber,
            pDataPacket->_header._packetTotal
        );
        free(buffer);
        return FALSE;
    }
    // Check the packet size coherency.
    if( (pDataPacket->_header._payloadSize == 0) ||
        (pDataPacket->_header._payloadSize >
            (MAX_PACKET_SIZE - sizeof(tPacketSize))) )
    {
        fprintf(
            stderr,
            "Invalid packet size: %u == 0 || > "
                NUM_2_STR(MAX_PACKET_SIZE - sizeof(tPacketSize)) ".\n",
            pDataPacket->_header._packetNumber
        );
        free(buffer);
        return FALSE;
    }
    // On success.
    return TRUE;
}

void closeClient(const int sd)
{
    if(close(sd) != 0){
        perror("Error closing socket");
        exit(EXIT_FAILURE);
    }
}