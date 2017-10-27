/*
 * Copyright 2017
 * MIT Licence
 * server.h
 * Header file for server.c
 */

#ifndef SERVER_H 
#define SERVER_H

// link properly with C++
#ifdef _cplusplus
extern "C" {
#endif // _cplusplus

// includes
#include "config.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <stdbool.h>
#include <stdint.h>
#include <glib.h>
#include <arpa/inet.h>
#include "representation.h"

// declarations

// stores a message and the IP address it was from
typedef struct {
    Message msg; // Error Message
    struct in_addr address; // IPv4 address which sent (or generated) the error
} BufferItem;

// stores information about an active connection
typedef struct {
    int fd;
    pthread_mutex_t mutex;
    struct sockaddr_in addr;
    time_t last_keep_alive;
} ConnectionData;

// result from reading from a socket (not used externally)
typedef enum {
    SUCCESS,
    ERROR,
    END
} ReadStatus;

// frees a BufferItem
void free_bufferitem(BufferItem *item);

// frees a ConnectionData
void free_connectiondata(ConnectionData *condata);

// set up the server
// returns success
bool start_server(const struct sockaddr *addr, socklen_t addrlen);

// read in an error message from the queue
// returns NULL immediately if there is no message to read in
BufferItem *read_message(void);

// stop the server safely
void stop_server(void);

#ifdef _cplusplus
}
#endif // _cplusplus
#endif // SOCKETS_H
