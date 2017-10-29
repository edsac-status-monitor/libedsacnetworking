/*
 * Copyright 2017
 * MIT Licensed
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

// tests for a keep_alive failure
static void test_keep_alive_fail(void) {
    // start server
    struct sockaddr *addr = alloc_addr("127.0.0.1", 3003);
    assert(NULL != addr);
    assert(true == start_server(addr, sizeof(*addr)));
    
    // connect to the server and don't send any KEEP_ALIVE messages
    int sending_fd = socket(AF_INET, SOCK_STREAM, 0);
    assert(-1 != sending_fd);
    // create tcp connection
    assert(-1 != connect(sending_fd, addr, sizeof(*addr)));
    
    // wait for the error to be detected
    strict_sleep(TEST_PAUSE);
    
    // get the error message from the server
    BufferItem *item = read_message();
    assert(NULL != item);
    
    // make sure it is due to a KEEP_ALIVE message
    assert(SOFT_ERROR == item->msg.type);
    assert(0 == strncmp("Connection timeout", item->msg.data.software.message->str, 19));
    
    free(addr);
    free_bufferitem(item);
    close(sending_fd);
    stop_server();
    puts("keep_alive_fail passed");
}

int main(void) {
    test_keep_alive_fail();

    return EXIT_SUCCESS;
}