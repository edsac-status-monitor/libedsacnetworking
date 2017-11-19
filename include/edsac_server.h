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
#include <sys/types.h>
#include <sys/socket.h>
#include <stdbool.h>
#include <stdint.h>
#include <glib.h>
#include <arpa/inet.h>
#include "edsac_representation.h"
#include "edsac_sending.h"
#include <time.h>

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
    time_t recv_time; // the time at which the message was received
} BufferItem;

// frees a BufferItem
void free_bufferitem(BufferItem *item);

// set up the server
// returns success
bool start_server(const struct sockaddr *addr, socklen_t addrlen);

// read in an error message from the queue
// returns NULL immediately if there is no message to read in
BufferItem *read_message(void);

// returns a list of IP addresses (sockaddr_in) we are currently connected to
GSList *get_connected_list(void);

// stop the server safely
void stop_server(void);

#ifdef _cplusplus
}
#endif // _cplusplus
#endif // SOCKETS_H
