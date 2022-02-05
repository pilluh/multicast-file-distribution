#include "server.h"
#include "types.h"
#include "constantes.h" /* MAX_PACKET_SIZE */
#include "macros.h"     /* NUM_2_STR */
#include <stdlib.h>     /* EXIT_FAILURE */
#include <stdio.h>      /* printf, fprintf, perror, stderr */
#include <assert.h>     /* assert, _Static_assert */
#include <string.h>     /* memset */
// Socket includes
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

// This is defined in milliseconds. 
#define THROT_WINDOW 100
// This is defined in kilobits per second (not kilobytes). 
#define THROT_BW 700

void initServer(tMultServer* const server, const char* const localAddr,
    const char* const multAddr, const uint16_t port)
{
    assert(server != NULL);
    server->_sd = socket(AF_INET, SOCK_DGRAM, 0);
    if(server->_sd < 0){
        perror("Opening datagram socket error");
        exit(EXIT_FAILURE);
    }else{
        printf("Socket opened, starting MCAST distribution !\n");
    }
    memset(&(server->_groupSock), 0, sizeof(server->_groupSock));
    server->_groupSock.sin_family = AF_INET;
    // FIXME : Externalize the multicast address and/or get it from the command line. 
    server->_groupSock.sin_addr.s_addr = inet_addr(multAddr);
    server->_groupSock.sin_port = htons(port);
    // We don't want to receive our own datagrams on the loopback so we disable it. 
    char loopch = 0;
    if(setsockopt(server->_sd, IPPROTO_IP, IP_MULTICAST_LOOP, (char *) &loopch,
        sizeof(loopch)) < 0)
    {
        perror("Disabling loopback failed. Bailing out !");
        close(server->_sd);
        exit(EXIT_FAILURE);
    }
    // FIXME : Get this IP address from the first available interface or from the command line. 
    struct in_addr localInterface;
    localInterface.s_addr = inet_addr(localAddr);
    if(setsockopt(server->_sd, IPPROTO_IP, IP_MULTICAST_IF,
        (char *) &localInterface, sizeof(localInterface)) < 0)
    {
        perror("Local interface can't be set, bailing out !");
        close(server->_sd);
        exit(EXIT_FAILURE);
    }
}

void runServer(tMultServer* const server,
               const tDataBlock* const pDataBlock,
               const tBlockNumber nDataBlock)
{
    assert(server != NULL);
    // FIXME : Implement throttling. 
    // Calculating throttling values
    _Static_assert(
        (THROT_WINDOW <= 1000),
        "Sorry man but THROT WINDOW must be under 1 second."
    );
    size_t throtData = ((THROT_BW * 1000 / 8) * THROT_WINDOW) / 1000;
    if(throtData <= sizeof(tDataBlockHeader)){
        printf("%zu", throtData);
        perror("Sorry man but I'd like at least to be able to send header "
            "without throttling it ! Yes, lazy am I !");
        exit(EXIT_FAILURE);
    }
    if(throtData > MAX_PACKET_SIZE){
        fprintf(
            stderr,
            "Packet size is exceeding maximum authorized: "
                "%zu > " NUM_2_STR(MAX_PACKET_SIZE) ".\n",
            throtData
        );
        exit(EXIT_FAILURE);
    }
    throtData -= sizeof(tDataBlockHeader);
    printf("Starting transmission... Press CTRL + C to interrupt.\n");
    tDataPacket packet = {{0, 0, 0, 0, 0, 0}, NULL};
    tBlockSize blockSize = 0;
    tBlockNumber i;
    tPacketNumber j;
#if BLOCK_SEND_REPEAT != 1
    uint8_t k = 1;
    // Prevent wrong code logic and infinite block sending.
    _Static_assert(
        BLOCK_SEND_REPEAT >= 1,
        "Unexpected value for BLOCK_SEND_REPEAT constant (< 1)."
    );
    // Prevent integer overflow.
    _Static_assert(
        BLOCK_SEND_REPEAT <= UINT8_MAX,
        "BLOCK_SEND_REPEAT constant exceeds UINT8_MAX value."
    );
#endif /* BLOCK_SEND_REPEAT != 1 */
    for(;;){
        for(i = 0; i < nDataBlock;
#if BLOCK_SEND_REPEAT != 1
            (k == BLOCK_SEND_REPEAT) ? ++i, k = 1 : ++k
#else
            ++i
#endif /* BLOCK_SEND_REPEAT != 1 */
        )
        {
            const tDataBlock* const pBlock = &(pDataBlock[i]);
            const tBlockSize nbThrotChunks =
                (pBlock->_header._payloadSize / throtData) + 1;
            if(nbThrotChunks > MAX_PACKET_NUMBER){
                fprintf(
                    stderr,
                    "Number of packets exceeds maximum authorized: "
                        "%zu > " NUM_2_STR(MAX_PACKET_NUMBER) ".\n",
                    nbThrotChunks
                );
                // Wait to adapt output bitrate (even when no packets are sent).
                usleep(THROT_WINDOW * 1000 * nbThrotChunks);
                continue;
            }
            for(j = 0; j < nbThrotChunks; ++j){
                // Fill header values.
                packet._header._blockNumber = pBlock->_header._blockNumber;
                packet._header._blockTotal = nDataBlock;
                packet._header._checksum = pBlock->_header._checksum;
                packet._header._packetNumber = j;
                packet._header._packetTotal = (tPacketNumber) nbThrotChunks;
                packet._header._payloadSize = (tPacketSize)
                    ((blockSize + throtData) <= pBlock->_header._payloadSize) ?
                        throtData :
                        pBlock->_header._payloadSize - blockSize;
                packet._header._padding = 0;
                // Allocate buffer.
                const tPacketSize packetSize =
                    sizeof(tDataPacketHeader) + packet._header._payloadSize;
                void* const buffer = malloc(packetSize);
                if(buffer == NULL){
                    fprintf(
                        stderr,
                        "Fail to allocate memory at %s line %d.\n",
                        __FILE__, __LINE__
                    );
                    blockSize += packet._header._payloadSize;
                    continue;
                }
                // Copy header.
                memcpy(buffer, &(packet._header), sizeof(packet._header));
                // Copy payload.
                memcpy(
                    buffer + sizeof(packet._header),
                    pBlock->_pPayload + blockSize,
                    packet._header._payloadSize
                );
                if(sendto(server->_sd, buffer, packetSize, 0,
                    (struct sockaddr *) &(server->_groupSock),
                    sizeof(server->_groupSock)) < 0)
                {
                    perror("Error sending packet data");
                }
                // Increment payload size for next calls.
                blockSize += packet._header._payloadSize;
                // Free buffer.
                free(buffer);
                // Wait to adapt output bitrate.
                usleep(THROT_WINDOW * 1000);
            }
            // Reset block size for each block.
            blockSize = 0;
	}
    }
}

int writePacket(tMultServer* const server, tDataPacket* const pDataPacket)
{
    assert((server != NULL) && (pDataPacket != NULL));
    // Rewrite packet in a single buffer.
    const size_t bufferSize =
        sizeof(pDataPacket->_header) + pDataPacket->_header._payloadSize;
    void* const buffer = malloc(bufferSize);
    if(buffer == NULL){
        fprintf(
            stderr,
            "Fail to allocate memory at %s line %d.\n", __FILE__, __LINE__
        );
        return EXIT_FAILURE;
    }
    // Copy header.
    memcpy(buffer, &(pDataPacket->_header), sizeof(pDataPacket->_header));
    // Copy payload.
    memcpy(buffer + sizeof(pDataPacket->_header), pDataPacket->_pPayload,
        pDataPacket->_header._payloadSize);
    // Send data.
    if(sendto(server->_sd,
        buffer,
        ((THROT_BW * 1000 / 8) * THROT_WINDOW) / 1000, 0,
        (struct sockaddr *) &(server->_groupSock),
        sizeof(server->_groupSock)) < 0)
    {
        perror("Error sending packet data");
        // Free packet intermediate buffer.
        free(buffer);
        return EXIT_FAILURE;
    }
    // Free packet intermediate buffer.
    free(buffer);
    return EXIT_SUCCESS;
}

void closeServer(tMultServer* const server)
{
    assert(server != NULL);
    if(close(server->_sd) != 0){
        perror("Error closing socket");
        exit(EXIT_FAILURE);
    }
}