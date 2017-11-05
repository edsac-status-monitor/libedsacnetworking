/*
 * Copyright 2017
 * GPL3 Licensed
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
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <errno.h>

// file descriptor for the TCP connection to the remote host
static int sending_fd = -1;
static pthread_mutex_t fd_mux = PTHREAD_MUTEX_INITIALIZER;

// locking has to be done first but this will unlock
static bool send_encoded_message(const char* encoded) {
    // send the encoded message
    size_t expected_count = strnlen(encoded, MAX_ENCODED_LEN);
    ssize_t count = write(sending_fd, encoded, expected_count);
    const int write_errno = errno; // incase pthread_mutex_unlock changes errno
    int err = pthread_mutex_unlock(&fd_mux);
    if (0 != err) {
        perror("Couldn't unlock sending mutex");
        exit(EXIT_FAILURE);
    }
    if (count != (ssize_t) expected_count) {
        printf("Error writing message to socket. expected_count = %li, count = %li, errno = %i, %s\n",
            expected_count, count, write_errno, strerror(write_errno));
        return false;
    }

    return true;
}

// called periodically to send a KEEP_ALIVE message
static void send_keep_alive(__attribute__((unused)) int compulsory) {
    const char* keep_alive_msg = "{\"version\":2,\"data\":{},\"type\":\"KEEP_ALIVE\"}";

    // set up the next alarm
    alarm(KEEP_ALIVE_INTERVAL);

    // if we interrupt the thread owning the mutex then a _lock() would deadlock
    if (0 != pthread_mutex_trylock(&fd_mux)) {
        perror("Can't lock mutex");
        return;
        // the server allows several missed keep_alive messages before it errors so this is probably fine
    }

    send_encoded_message(keep_alive_msg); // unlocks mutex
}

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

    // periodically send KEEP_ALIVE message
    if (SIG_ERR == signal(SIGALRM, send_keep_alive)) {
        close(sending_fd);
        sending_fd = -1;
        return false;
    }
    alarm(KEEP_ALIVE_INTERVAL);

    return true;
}

bool send_message(const Message *msg) {
    // msg checked for null in encode_message

    // encode the message for transmission
    char *encoded = NULL;
    encode_message(msg, &encoded);
    if (!encoded)
        return false;

    // lock mutex
    if (-1 == pthread_mutex_lock(&fd_mux)) {
        perror("failed to lock sending mutex");
        free(encoded);
        return false;
    }

    bool ret = send_encoded_message(encoded);    

    free(encoded);

    return ret;
}

void stop_sending(void) {
    if (-1 != sending_fd)
        close(sending_fd);

    struct sigaction sa;
    DISABLE_SIGNAL(SIGALRM)
}