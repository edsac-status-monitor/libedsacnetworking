/*
 * Copyright 2017
 * MIT Licensed
 * test/server.c
 * Test suite for sockets.c
 */

// includes
#include "config.h"
#include "server.h"
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

// functions
int main(void) {
    struct sockaddr *addr = alloc_addr("127.0.0.1", 2000);
    assert(NULL != addr);

    if (!start_server(addr, sizeof(*addr))) {
        puts("failed to start server");
        free(addr);
        stop_server();
        return EXIT_FAILURE;
    }
    free(addr);

    while(true);
    stop_server();
    return EXIT_SUCCESS;
}