/* 
 * File:   main.c
 * Author: pilluh
 *
 * Created on 28 décembre 2015, 14:54
 */

#include <stdint.h>         /* uint16_t */
#include <stdio.h>          /* fprintf, stderr */
#include <stdlib.h>         /* EXIT_FAILURE, EXIT_SUCCESS */
#include <string.h>         /* strcmp */
#include "constantes.h"     /* DEF_BLOCK_SIZE, PREPARE_OPTION, TRANSMIT_OPTION,
                                RECEIVE_OPTION */
#include "splitfile.h"      /* splitFile */
#include "transmitfile.h"   /* transmitFile */
#include "receivefile.h"    /* receiveFile */

/*
 * 
 */
int main(int argc, char** argv)
{
    // Check mandatory parameters.
    if(argc < 3){
        // Print usage.
        printf(
            "Usage: %s <"PREPARE_OPTION"|"TRANSMIT_OPTION"|"RECEIVE_OPTION"> "
                "<input-file> <output-dir>=%s (<block-size>=%zu | | "
                "<multi-addr>=%s <local-addr>=%s <port>=%d)\n",
            argv[0], DEF_DATA_DIRECTORY, DEF_BLOCK_SIZE, DEF_MULTI_ADDR,
                DEF_LOCAL_ADDR, DEF_PORT_NUMBER
        );
        return (EXIT_FAILURE);
    }
    // Get desired option.
    const char* const option = argv[1];
    // Get input filename value.
    const char* const inputFileName = argv[2];
    // Get output directory value.
    const char* const outputDir = (argc >= 4) ? argv[3] : DEF_DATA_DIRECTORY;
    if(strcmp(option, PREPARE_OPTION) == 0){
        // Get block size value.
        size_t blockSize = DEF_BLOCK_SIZE;
        if(argc >= 5){
            blockSize = strtoul(argv[4], NULL, 10);
            if(blockSize == 0L){
                fprintf(stderr, "Invalid block size: '%s'.\n", argv[4]);
                return (EXIT_FAILURE);
            }
        }
        splitFile(inputFileName, outputDir, blockSize);
    }else if( (strcmp(option, TRANSMIT_OPTION) == 0) ||
        (strcmp(option, RECEIVE_OPTION) == 0) )
    {
        // Get multicast-address.
        const char* const multAddr =
            (argc >= 5) ? argv[4] : DEF_MULTI_ADDR;
        // Get local address.
        const char* const localAddr =
            (argc >= 6) ? argv[5] : DEF_LOCAL_ADDR;
        // Get port.
        unsigned long port = DEF_PORT_NUMBER;
        if(argc >= 7){
            port = strtoul(argv[6], NULL, 10);
            if((port == 0L) || (port >= 65536L)){
                fprintf(stderr, "Invalid port number: '%s'.\n", argv[6]);
                return (EXIT_FAILURE);
            }
        }
        if(strcmp(option, TRANSMIT_OPTION) == 0){
            transmitFile(outputDir, localAddr, multAddr, (uint16_t) port);
        }else{
            receiveFile(inputFileName, outputDir, localAddr, multAddr,
                (uint16_t) port);
        }
    }else{
        fprintf(
            stderr,
            "Invalid option: '%s' "
                "("PREPARE_OPTION"|"TRANSMIT_OPTION"|"RECEIVE_OPTION").\n",
            argv[1]
        );
        return (EXIT_FAILURE);
    }
    return (EXIT_SUCCESS);
}

