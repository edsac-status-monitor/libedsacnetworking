/*
 * Copyright 2017
 * GPL3 Licensed
 * test/system.c
 * system test covering server.c and sending.c
 */

// includes
#include "config.h"
#include "server.h"
#include "representation.h"
#include "sending.h"
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

// creates a server and client and tests that messages can be sent successfully between them
int main(void) {
    // sent as the body of a software error
    const char *test_message = "hello world!";

    struct sockaddr *addr = alloc_addr("127.0.0.1", 2000);
    assert(NULL != addr);

    Message msg;
    software_error(&msg, test_message);

    puts("starting server");
    if (!start_server(addr, sizeof(*addr))) {
        perror("failed to start server");
        free(addr);
        return EXIT_FAILURE;
    }

    puts("connecting to server");
    if (!start_sending(addr, sizeof(*addr))) {
        perror("failed to start sending");
        free(addr);
        return EXIT_FAILURE;
    }

    free(addr);
    addr = NULL;

    puts("sending message");
    if (!send_message(&msg)) {
        perror("failed to send message");
        return EXIT_FAILURE;
    }

    puts("disconnecting");
    stop_sending();

    // delay so that all the threads are finished
    usleep(500);
    
    // get first message from the queue
    BufferItem *soft_err = read_message(); // if this is failing then first try increasing TIMING_DELAY
    assert(NULL != soft_err);
    // same type
    assert(soft_err->msg.type == msg.type);
    // same content
    assert(0 == strncmp(test_message, soft_err->msg.data.software.message->str, strlen(test_message)));
    free_bufferitem(soft_err);
    
    // get the disconnect message from the queue
    BufferItem *disconnect = read_message();
    assert(NULL != disconnect);
    
    // same type
    assert(SOFT_ERROR == disconnect->msg.type);
    // same content
    assert(0 == strncmp("Connection closed", disconnect->msg.data.software.message->str, 18));
    free_bufferitem(disconnect);
    
    free_message(&msg);

    puts("stopping server");
    stop_server();

    puts("passed");
    return EXIT_SUCCESS;
}