/*
 * Copyright 2017
 * MIT Licensed
 * test/server.c
 * Starts listening for new messages and then sleeps forever
 */

// includes
#include "config.h"
#include <stdlib.h>
#include <assert.h>
#include "server.h"
#include "representation.h"
#include <unistd.h>
#include <stdio.h>

// functions
int main(int argc, char** argv) {
    const char *default_addr = "127.0.0.1";
    const uint16_t default_port = 2000;

    struct sockaddr *addr = NULL;
    if (1 == argc) {
        addr = alloc_addr(default_addr, default_port);
    } else if (3 == argc) {
        char *invalid;
        long int port = strtol(argv[2], &invalid, 10);
        assert('\0' == *invalid);
        addr = alloc_addr(argv[1], (uint16_t) port);
    } else {
        printf("USAGE: %s [IP port]\n", argv[0]);
        printf("The default IP=%s and port=%i\n", default_addr, (int) default_port);
        return EXIT_FAILURE;
    }
    assert(NULL != addr);

    assert(true == start_server(addr, sizeof(*addr)));

    free(addr);

    puts("Listening");

    while(true) {
        sleep(1000);
    }

    stop_server();

    return EXIT_SUCCESS;
}
