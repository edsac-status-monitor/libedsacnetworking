/*
 * Copyright 2017
 * GPL3 Licensed
 * keep_alive.c
 * Unit test for keep_alive messages
 */

// includes
#include "config.h"
#include "edsac_server.h"
#include "edsac_arguments.h"
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// functions

#define TEST_PAUSE ((KEEP_ALIVE_PROD) * 2)

// looping because it always seems to wake up early
static void strict_sleep(unsigned int left_to_sleep) {
    while (0 != left_to_sleep) {
        //printf("%li seconds left to sleep\n", (long int) left_to_sleep);
        left_to_sleep = sleep(left_to_sleep);
    }
}

// runs a server and client long enough to be sure that no keep_alive errors are being run
int main(void) {
    // start server
    struct sockaddr *addr = alloc_addr("127.0.0.1", 4002);
    assert(NULL != addr);
    assert(true == start_server(addr, sizeof(*addr)));

    // start sender
    assert(true == start_sending(addr, sizeof(*addr)));
    free(addr);

    // wait long enough that a keep alive failure would have occured if no KEEP_ALIVE messages were sent
    strict_sleep(TEST_PAUSE);

    // check for connection errors
    assert(NULL == read_message());
    puts("keep_alive_pass succeeded");

    stop_sending();
    stop_server();
    return EXIT_SUCCESS;
}