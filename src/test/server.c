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
int main(void) {
    struct sockaddr *addr = alloc_addr("127.0.0.1", 2000);
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
