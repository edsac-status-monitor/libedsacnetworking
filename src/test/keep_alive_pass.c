/*
 * Copyright 2017
 * GPL3 Licensed
 * keep_alive.c
 * Unit test for keep_alive messages
 */

// includes
#include "config.h"
#include "server.h"
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
        printf("%li seconds left to sleep\n", (long int) left_to_sleep);
        left_to_sleep = sleep(left_to_sleep);
    }
}

// signal handler or SIGTERM to clean up the sender process
static void sigterm_cleanup(__attribute__((unused)) int compulsory) {
    stop_sending();
}

// runs a server and client long enough to be sure that no keep_alive errors are being run
int main(void) {
    // start server
    struct sockaddr *addr = alloc_addr("127.0.0.1", 4002);
    assert(NULL != addr);
    assert(true == start_server(addr, sizeof(*addr)));

    // we need the server and sender to live in different processes because they both use SIGALRM:
    pid_t child = fork();
    switch(child) {
        case -1:
            puts("Fork failed");
            return EXIT_FAILURE;
        case 0:
            puts("Child started");
            // start sender
            assert(true == start_sending(addr, sizeof(*addr)));
            free(addr);
            // set up a signal handler for sigterm to clean up
            signal(SIGTERM, sigterm_cleanup);
            // wait for the main process to kill us
            while(true) {
                sleep(100);
            }
            return EXIT_FAILURE; // we shouldn't get here
        default:
            // still in server process. Continue:
            free(addr);
    }

    // wait long enough that a keep alive failure would have occured if no KEEP_ALIVE messages were sent
    strict_sleep(TEST_PAUSE);

    // check for connection errors
    assert(NULL == read_message());
    puts("keep_alive_pass succeeded");

    // kill the child process
    kill(child, SIGTERM);

    stop_server();
    puts("Server stopped");
    return EXIT_SUCCESS;
}