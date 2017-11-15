/*
 * Copyright 2017
 * GPL3 Licensed
 * arguments.c
 * Command line argument parsing
 */

// includes
#include "config.h"
#include "edsac_arguments.h"
#include <glib.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// functions

#define RET_IF_NULL(_x) if (NULL == _x) return NULL

struct sockaddr *get_args(int *argc, char ***argv, GOptionGroup *others) {
    const uint16_t default_port = 2000;
    const char *default_addr = "127.0.0.1";

    // place to put the command line arguments we got
    bool use_addr_str = true;
    char *addr_str = NULL;
    gint port = -1;

    // definition of command line arguments
    GOptionEntry entries[] = {
        {"address", 'a', G_OPTION_FLAG_NONE, G_OPTION_ARG_STRING, &addr_str, "IPv4 Address", "w.x.y.z"},
        {"port", 'p', G_OPTION_FLAG_NONE, G_OPTION_ARG_INT, &port, "TCP Port", "PORT"},
        {NULL}
    };

    // set up
    GOptionContext *context = g_option_context_new(NULL);
    RET_IF_NULL(context);
    g_option_context_add_main_entries(context, entries, NULL);

    if (NULL != others) {
        g_option_context_add_group(context, others);
    }

    // parse command line arguments
    if (!g_option_context_parse(context, argc, argv, NULL)) {
        g_free(addr_str);
        g_option_context_free(context);
        return NULL;
    }

    // try to use what we got

    // default values
    if (NULL == addr_str) {
        use_addr_str = false;
    }
    if (-1 == port) {
        port = default_port;
    }

    // try to turn this into a sockaddr
    struct sockaddr *addr;
    if (use_addr_str) {
        addr = alloc_addr(addr_str, (uint16_t) port);
    } else {
        addr = alloc_addr(default_addr, (uint16_t) port);
    }

    // clean up
    if (use_addr_str) {
        g_free(addr_str);
    }
    g_option_context_free(context);

    return addr;
}

// sets up a sockaddr structure for the IPv4 address described in addr e.g. addr="127.0.0.1"
struct sockaddr *alloc_addr(const char *addr, uint16_t port) {
    if (NULL == addr)
        return NULL;

    // allocate the sockaddr structure
    struct sockaddr_in *ret = (struct sockaddr_in *) malloc(sizeof(struct sockaddr_in));
    if (NULL == ret)
        return NULL;

    // clear its contents
    memset(ret, 0, sizeof(struct sockaddr_in));

    // set up the sockaddr
    ret->sin_family = AF_INET; // address family
    ret->sin_port = htons(port); // port (in network byte order)

    // the address
    if (1 != inet_pton(AF_INET, addr, (void *) &(ret->sin_addr))) {
        free(ret);
        return NULL;
    }

    // sockaddr is the general type for any sockaddr_*
    return (struct sockaddr *) ret;
}

