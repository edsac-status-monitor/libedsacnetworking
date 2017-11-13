/*
 * Copyright 2017
 * GPL3 License
 * sending.h
 * header file for sending.c
 */

#ifndef TEMPLATE_H 
#define TEMPLATE_H

// link properly with C++
#ifdef _cplusplus
extern "C" {
#endif // _cplusplus

// includes
#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "edsac_representation.h"

// declarations

// the time between KEEP_ALIVE messages in seconds
#define KEEP_ALIVE_INTERVAL 10

// addr is the address of the server to which we will report errors
bool start_sending(const struct sockaddr *addr, socklen_t addrlen);

bool send_message(const Message *msg);

void stop_sending(void);

#ifdef _cplusplus
}
#endif // _cplusplus
#endif // TEMPLATE_H
