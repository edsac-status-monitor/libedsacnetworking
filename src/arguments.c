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
#include <stdio.h>

// functions

struct sockaddr *get_args(int *argc, char ***argv, GOptionGroup *other_group, GOptionEntry *other_entries) {
    const uint16_t default_port = 2000;
    const char *default_addr = "127.0.0.1";

    if (NULL != other_group) {
        g_option_group_ref(other_group);
    }

    // place to put the command line arguments we got
    bool use_addr_str = true;
    char *addr_str = NULL;
    gint port = -1;

    GOptionGroup *networking_group = g_option_group_new("networking", "Options relating to networking", "Options relating to networking", NULL, NULL);
    assert(NULL != networking_group);

    // definition of command line arguments for networking group
    GOptionEntry entries[] = {
        {"address", 'a', G_OPTION_FLAG_NONE, G_OPTION_ARG_STRING, &addr_str, "IPv4 Address", "w.x.y.z"},
        {"port", 'p', G_OPTION_FLAG_NONE, G_OPTION_ARG_INT, &port, "TCP Port", "PORT"},
        {NULL}
    };
    g_option_group_add_entries(networking_group, entries);

    // set up GOptionContext
    GOptionContext *context = g_option_context_new(NULL);
    assert(NULL != context);

    g_option_context_add_group(context, networking_group);
    if (NULL != other_entries) {
        g_option_context_add_main_entries(context, other_entries, NULL);
    }

    if (NULL != other_group) {
        g_option_context_add_group(context, other_group);
    }

    // parse command line arguments
    if (!g_option_context_parse(context, argc, argv, NULL)) {
        g_free(addr_str);
        g_option_context_free(context);
        g_option_group_unref(networking_group);
        if (NULL != other_group) {
            g_option_group_unref(other_group);
        }

        puts("Could not parse arguments");
        puts(g_option_context_get_help(context, TRUE, NULL));
        exit(EXIT_FAILURE);
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
    struct sockaddr *addr = NULL;
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
    g_option_group_unref(networking_group);
    if (NULL != other_entries) {
        g_option_group_unref(other_group);
    }

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

