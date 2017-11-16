/*
 * Copyright 2017
 * GPL3 Licensed
 * edsac_arguments.h
 * Command line argument processing to extract parameters specifying an IPv4 address and a port number
 */

#ifndef EDSAC_ARGUMENTS_H 
#define EDSAC_ARGUMENTS_H

// link properly with C++
#ifdef _cplusplus
extern "C" {
#endif // _cplusplus

// includes
#include <arpa/inet.h>
#include <glib.h>

// declarations

// sets up a sockaddr structure based on arguments
struct sockaddr *get_args(int *argc, char ***argv, GOptionGroup *other_group, GOptionEntry *other_entries);

// sets up an ip v4 address structure
// returns NULL on failure. addr is a string representation e.g. "127.0.0.1"
// the returned address should be free()'ed after use
struct sockaddr *alloc_addr(const char *addr, uint16_t port);


#ifdef _cplusplus
}
#endif // _cplusplus
#endif // EDSAC_ARGUMENTS_H
