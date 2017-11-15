/*
 * Copyright 2017
 * GPL3 Licensed
 * test/server.c
 * Starts listening for new messages and then sleeps forever
 */

// includes
#include "config.h"
#include <stdlib.h>
#include <assert.h>
#include "edsac_server.h"
#include "edsac_representation.h"
#include "edsac_arguments.h"
#include <unistd.h>
#include <stdio.h>

// functions
int main(int argc, char** argv) {
    struct sockaddr *addr = get_args(&argc, &argv, NULL);
    assert(NULL != addr);

    assert(true == start_server(addr, sizeof(*addr)));

    free(addr);

    puts("Listening");

    while(true) {
        sleep(1000);
    }

    puts("stopping server");
    stop_server();

    return EXIT_SUCCESS;
}
