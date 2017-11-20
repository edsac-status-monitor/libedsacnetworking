/*
 * Copyright 2017
 * GPL3 Licensed
 * sending_demo.c
 * Interactive sending of error messages
 */

// includes
#include "config.h"
#include <stdlib.h>
#include <assert.h>
#include "edsac_sending.h"
#include "edsac_representation.h"
#include "edsac_arguments.h"
#include <unistd.h>
#include <stdio.h>

// handle CTR+C
static void sigint_handler(__attribute__((unused)) int sig) {
    puts(""); // newline
    stop_sending();
    exit(EXIT_SUCCESS);
}

// functions
int main(int argc, char** argv) {
    struct sockaddr *addr = get_args(&argc, &argv, NULL, NULL);
    assert(NULL != addr);

    assert(true == start_sending(addr, sizeof(*addr)));

    free(addr);

    puts("Connected");

    // SIGINT (CTL+C) handler
    signal(SIGINT, sigint_handler);

    // continually get and send messages
    Message msg;
    msg.type = SOFT_ERROR;
    msg.data.software.message = g_string_new(NULL);

    static char buf[128] = {'\0'};

    while(true) {
        printf("Enter an error message: ");
        if (fgets(buf, sizeof(buf), stdin) != buf) {
            buf[0] = '\0';
        }
        msg.data.software.message = g_string_assign(msg.data.software.message, buf);
        assert(NULL != msg.data.software.message);
        
        // remove newline character
        msg.data.software.message = g_string_overwrite(msg.data.software.message, msg.data.software.message->len - 1, " ");

        send_message(&msg);
    }

    perror("Unreachable!");

    // never run. Just here incase this is used as an example
    g_free(msg.data.software.message);
    stop_sending();

    return EXIT_FAILURE;
}

