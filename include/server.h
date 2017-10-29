/*
 * Copyright 2017
 * GPL3 License
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
#include "sending.h"

// declarations

// how many KEEP_ALIVE messages between checks
#define KEEP_ALIVE_CHECK_PERIOD 2 

// how many consecutive failed KEEP_ALIVE checks before an error
#define KEEP_ALIVE_GRACE 3

// the actual time in seconds before an error is signaled
#define KEEP_ALIVE_PROD ((KEEP_ALIVE_CHECK_PERIOD) * (KEEP_ALIVE_GRACE) * (KEEP_ALIVE_INTERVAL))

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

// used in server.c and sending.c
#define DISABLE_SIGNAL(_signal) \
    memset(&sa, 0, sizeof(sa)); \
    sa.sa_handler = SIG_DFL; \
    if (-1 == sigaction(_signal, &sa, NULL)) { \
        puts("Couldn't disable signal"); \
    } 

#ifdef _cplusplus
}
#endif // _cplusplus
#endif // SOCKETS_H
