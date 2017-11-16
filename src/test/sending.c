/*
 * Copyright 2017
 * GPL3 Licensed
 * test/sending.c
 * Establishes a sending connection and then sleeps forever
 */

// includes
#include "config.h"
#include <stdlib.h>
#include <assert.h>
#include "edsac_sending.h"
#include "edsac_representation.h"
#include "edsac_arguments.h"
#include <unistd.h>
#include <stdio.h>

// functions
int main(int argc, char** argv) {
    struct sockaddr *addr = get_args(&argc, &argv, NULL, NULL);
    assert(NULL != addr);

    assert(true == start_sending(addr, sizeof(*addr)));

    free(addr);

    puts("Connected");

    while(true) {
        pause();
    }

    perror("Unreachable!");

    // never run. Just here incase this is used as an example
    stop_sending();

    return EXIT_FAILURE;
}
