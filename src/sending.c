/*
 * Copyright 2017
 * MIT Licensed
 * sending.c
 * functions related to sending data to a remote host
 */

// includes
#include "config.h"
#include "sending.h"
#include "server.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <stdlib.h>

// file descriptor for the TCP connection to the remote host
static int sending_fd = -1;

bool start_sending(const struct sockaddr *addr, socklen_t addrlen) {
    // open a socket
    sending_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == sending_fd) {
        return false;
    }

    // create tcp connection
    if (-1 == connect(sending_fd, addr, addrlen)) {
        return false;
    }

    return true;
}

bool send_message(ErrorMessage *msg) {
    // msg checked for null in encode_message

    // encode the message for transmission
    char *encoded = NULL;
    encode_message(msg, &encoded);
    if (!encoded)
        return false;

    // send the encoded message
    size_t expected_count = strnlen(encoded, MAX_ENCODED_LEN);
    ssize_t count = write(sending_fd, encoded, expected_count);
    if (count != (ssize_t) expected_count) {
        free(encoded);
        return false;
    }

    free(encoded);
    return true;    
}

void stop_sending(void) {
    if (-1 != sending_fd)
        close(sending_fd);
}