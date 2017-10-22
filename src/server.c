/*
 * Copyright 2017
 * MIT Licensed
 * sockets.c
 * Functions for running the server
 */

// includes
#include "config.h"
#include "server.h"
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include "representation.h"
#include <errno.h>
#include <pthread.h>

#include <stdio.h>

#define CONNECT_SIG 1
#define READ_SIG 2

// global read buffer
static GQueue *read_buff = NULL;
static pthread_mutex_t read_buff_mux = PTHREAD_MUTEX_INITIALIZER;

// the listening socket
static int listen_socket = -1;

// sets up a fd for realtime signal IO using signal sig, handled by handler
static bool setup_rt_signal_io(int fd, int sig, void (*handler)(int, siginfo_t *, void *)) {
    // establish signal handler for new connections
    struct sigaction sa;
    sa.sa_sigaction = handler;
    sa.sa_flags = SA_RESTART | SA_SIGINFO; // SA_RESTART: don't break things so much when we interupt them, SA_SIGINFO: be realtime
    sigemptyset(&sa.sa_mask); // don't mask signals
    if (-1 == sigaction(sig, &sa, NULL)) { // install new settings for this signal
        return false;
    }

    // set the owner of the socket to us so that we get the signals
    if (-1 == fcntl(fd, F_SETOWN, getpid())) {
        return false;
    }

    // specify our handler instead of the default SIGIO handler
    // I am not sure why this is needed in addition to setting sa.sa_sigaction
    if (-1 == fcntl(fd, F_SETSIG, sig)) {
        return false;
    }

    // enable signaling & non-blocking IO
    int flags = fcntl(fd, F_GETFL);
    if (-1 == fcntl(fd, F_SETFL, flags | O_ASYNC | O_NONBLOCK)) {
        return false;
    }

    return true;
}

// attempt to read a full  json object from a fd (defined as "{*}", handling nesting)
static ReadStatus read_json_object(int fd, GString **out_string) {
    // assume that we can read the whole thing at once

    GString *string = g_string_new("{");

    // the first character must be '{'
    char buf;
    ssize_t num_read = read(fd, &buf, 1);
    if (1 != num_read) { // we could not read a whole character
        int e = errno;
        g_string_free(string, true);

        // if it looks like there just wasn't any data left in the buffer for us to read
        if ((EAGAIN == e) || (EWOULDBLOCK == e) || (0 == num_read)) {
            return END;
        }
        
        // there was data but an error occured
        return ERROR;
    }

    // the first character must be {
    if (buf != '{') {
        g_string_free(string, true);
        if ((buf == '\n') || (buf == 13)) // ignore newline characters so we can telnet in for testing
            return read_json_object(fd, out_string);

        printf("invalid c=%i\n", (int) buf);
        return ERROR;
    }

    // read in the rest
    int nest_count = 1;
    while (true) {
        // try to read 1 character
        num_read = read(fd, &buf, 1);
        if (1 != num_read) {
            int e = errno;
            g_string_free(string, true);

            // if we reached the end of the buffer
            if ((EAGAIN == e) || (EWOULDBLOCK == e) || (0 == num_read)) {
                puts("partial packet!");
                return ERROR;
            } else {
                // unknown error
                printf("errno=%i. num_read=%li\n", e, num_read);
                return ERROR;
            }
        }
        // we did read in a character successfully
        
        // nesting count
        if (buf == '{')
            nest_count += 1;
        else if (buf == '}')
            nest_count -= 1;

        // add the character to the string
        string = g_string_append_c(string, buf);

        // are we done?
        if (0 == nest_count)
            break;
    }

    *out_string = string;
    return SUCCESS;
}

// fetch a read buffer item from fd
static ReadStatus fetch_item(int fd) {
    // the item we will add to the buffer for this read
    BufferItem *item = malloc(sizeof(BufferItem));
    if (NULL == item) {
        return ERROR;
    }

    // attempt to read in the json object
    GString *obj;
    ReadStatus status = read_json_object(fd, &obj);
    if (SUCCESS != status) {
        free(item);
        return status;
    }

    item->object = obj;
    printf("read object %s\n", obj->str);

    // get the IP address of the remote host
    struct sockaddr addr;
    socklen_t addrlen = sizeof(addr);
    if (0 != getpeername(fd, &addr, &addrlen)) {
        free_bufferitem(item);
        return ERROR;
    }

    // check it is IPv4
    struct sockaddr_in *ip4_addr = (struct sockaddr_in *) &addr;
    if (AF_INET != ip4_addr->sin_family) {
        free_bufferitem(item);
        return ERROR;
    }

    item->address = ip4_addr->sin_addr;

    // get access to the queue
    if (0 != pthread_mutex_lock(&read_buff_mux)) {
        free_bufferitem(item);
        return ERROR;
    }

    // add the item to the queue
    g_queue_push_tail(read_buff, (gpointer) item);

    pthread_mutex_unlock(&read_buff_mux);
    return SUCCESS;
}

// thread to read in all of the objects waiting in a buffer
/* the actual reading is done in its own thread for the following situation:
 *    1. some normal code has read_buff_mux locked
 *    2. io_handler signal tries to lock read_buff_mux
 *    3. deadlock
 *
 * Using s different thread for the io_handler means that it can block waiting for read_buff_mux without creating a deadlock
 */
static void *object_reader_thread(void *arg) {
    int fd = *((int *) arg);

    // read in every available object
    while (true) {
        ReadStatus status = fetch_item(fd);
        if (ERROR == status) {
            puts("Read ERROR from remote host\n"); 
            close(fd);
            return NULL;
        } else if (END == status) {
            return NULL; // we have read all of it
        }
    }

    free(arg);
}

// realtime signal handler for when IO is available on a connection with a client
static void io_handler(__attribute__ ((unused)) int sig, __attribute__ ((unused)) siginfo_t *si, __attribute__ ((unused)) void *ucontext) {
    printf("IO! fd=%d\n", si->si_fd);

    // check to see if there is any data to read
    // MSG_PEEK so that the read character is not removed from the read buffer
    char buf;
    ssize_t count = recv(si->si_fd, &buf, 1, MSG_PEEK);
    if (1 != count) {
        printf("closed connection\n");
        close(si->si_fd);
        return;
    }

    // do the actual reading in a different thread (see comment on thread function)
    int *thread_arg = malloc(sizeof(int));
    if (!thread_arg)
        return;
    *thread_arg = si->si_fd;
    pthread_t thread;
    pthread_create(&thread, NULL, object_reader_thread, (void *) thread_arg);
}

// realtime signal handler for when there is a connection to the listening socket
static void connect_handler(__attribute__ ((unused)) int sig, siginfo_t *si, __attribute__ ((unused)) void *ucontext) {
    // accepts a connection from a remote host and sets it up to do signal driven IO
    printf("connect!\n");

    int fd = accept(si->si_fd, NULL, NULL);
    if (-1 == fd)
        return;

    setup_rt_signal_io(fd, SIGRTMIN + READ_SIG, io_handler);
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

bool start_server(const struct sockaddr *addr, socklen_t addrlen) {
    if (NULL == addr)
        return false;

    // create IPv4 TCP socket to communicate over
    // non-blocking so we can use signal driven IO
    // cloexec for security (closes fd on an exec() syscall)
    listen_socket = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (-1 == listen_socket)
        return false;

    // bind to the specified address
    if (-1 == bind(listen_socket, addr, addrlen)) {
        close(listen_socket);
        listen_socket = -1;
        return false;
    }

    // set up signal-driven IO on the listening socket
    if (!setup_rt_signal_io(listen_socket, SIGRTMIN + CONNECT_SIG, connect_handler)) {
        close(listen_socket);
        listen_socket = -1;
        return false;
    }

    // initialise the read buffer
    read_buff = g_queue_new();

    // begin listening on the socket
    if (-1 == listen(listen_socket, SOMAXCONN)) {
        close(listen_socket);
        listen_socket = -1;
        g_queue_free(read_buff);
        read_buff = NULL;
        return false;
    }

    return true;
}

// free a BufferItem
void free_bufferitem(BufferItem *item) {
    g_string_free(item->object, true);
    free(item);
}

void stop_server(void) {
    // close the open socket
    if (-1 != listen_socket) {
        close(listen_socket);
        listen_socket = -1;
    }

    // free up the read buffer
    if (read_buff)
        g_queue_free_full(read_buff, (GDestroyNotify) free_bufferitem);
}