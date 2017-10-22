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
    ErrorMessage msg; // Error Message
    struct in_addr address; // IPv4 address which sent (or generated) the error
} BufferItem;

// result from reading from a socket (not used externally)
typedef enum {
    SUCCESS,
    ERROR,
    END
} ReadStatus;

// frees a BufferItem
void free_bufferitem(BufferItem *item);

// sets up an ip v4 address structure
// returns NULL on failure. addr is a string representation e.g. "127.0.0.1"
// the returned address should be free()'ed after use
struct sockaddr *alloc_addr(const char *addr, uint16_t port);

// set up the server
// returns success
bool start_server(const struct sockaddr *addr, socklen_t addrlen);

// stop the server safely
void stop_server(void);

#ifdef _cplusplus
}
#endif // _cplusplus
#endif // SOCKETS_H
