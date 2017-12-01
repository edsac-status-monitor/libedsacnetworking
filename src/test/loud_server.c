/*
 * Copyright 2017
 * GPL3 Licensed
 * test/server.c
 * Starts listening for new messages and then sleeps forever
 */

// includes
#include "config.h"
#include <stdlib.h>
#include <assert.h>
#include "edsac_server.h"
#include "edsac_representation.h"
#include "edsac_arguments.h"
#include <unistd.h>
#include <stdio.h>

// functions
int main(int argc, char** argv) {
    struct sockaddr *addr = get_args(&argc, &argv, NULL, NULL);
    assert(NULL != addr);

    assert(true == start_server(addr, sizeof(*addr)));

    free(addr);

    puts("Listening");

    while(true) {
        BufferItem *item = read_message();
        if (NULL != item) {
            switch (item->msg.type) {
                case HARD_ERROR_OTHER:
                    puts(item->msg.data.hardware_other.message->str);
                    break;
                case HARD_ERROR_VALVE:
                    puts(item->msg.data.hardware_valve.message->str);
                    break;
                case SOFT_ERROR:
                    puts(item->msg.data.software.message->str);
                    break;
                default:
                    break;
            }
            free_bufferitem(item);
        }
        usleep(10);
    }

    puts("stopping server");
    stop_server();

    return EXIT_SUCCESS;
}
