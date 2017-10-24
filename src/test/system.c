/*
 * Copyright 2017
 * MIT Licensed
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

// some delays for threads to finish running
// needed because items are not added to the read buffer synchronously so if we rush into reading from the queue immediately, the message won't be there yet
#define TIMING_DELAY 1E5 // micro seconds

// creates a server and client and tests that messages can be sent successfully between them
int main(void) {
    // sent as the body of a software error
    const char *test_message = "hello world!";

    struct sockaddr *addr = alloc_addr("127.0.0.1", 2000);
    assert(NULL != addr);

    ErrorMessage msg;
    software_error(&msg, test_message);

    puts("starting server");
    if (!start_server(addr, sizeof(*addr))) {
        puts("failed to start server");
        free(addr);
        return EXIT_FAILURE;
    }

    puts("connecting to server");
    if (!start_sending(addr, sizeof(*addr))) {
        puts("failed to start sending");
        free(addr);
        return EXIT_FAILURE;
    }

    free(addr);
    addr = NULL;

    puts("sending message");
    if (!send_message(&msg)) {
        puts("failed to send message");
        return EXIT_FAILURE;
    }

    puts("disconnecting");
    stop_sending();

    // sleep for a bit while the server threads finish processing
    usleep(TIMING_DELAY);

    // get first message from the queue
    BufferItem *soft_err = read_message(); // if this is failing then first try increasing TIMING_DELAY
    assert(NULL != soft_err);
    // same type
    assert(soft_err->msg.type == msg.type);
    // same content
    assert(0 == strncmp(test_message, soft_err->msg.data.software.message, strlen(test_message)));

    // get the disconnect message from the queue
    BufferItem *disconnect = read_message();
    assert(NULL != disconnect);

    // same type
    assert(SOFT_ERROR == disconnect->msg.type);
    // same content
    assert(0 == strncmp("Connection closed", disconnect->msg.data.software.message, 18));

    puts("stopping server");
    stop_server();

    puts("passed");
    return EXIT_SUCCESS;
}