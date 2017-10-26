/*
 * Copyright 2017
 * MIT Licensed
 * test/sending.c
 * Establishes a sending connection and then sleeps forever
 */

// includes
#include "config.h"
#include <stdlib.h>
#include <assert.h>
#include "sending.h"
#include "representation.h"
#include <unistd.h>
#include <stdio.h>

// functions
int main(void) {
    struct sockaddr *addr = alloc_addr("127.0.0.1", 2000);
    assert(NULL != addr);

    assert(true == start_sending(addr, sizeof(*addr)));

    free(addr);

    puts("Connected");

    while(true) {
        sleep(1000);
    }

    stop_sending();

    return EXIT_SUCCESS;
}
