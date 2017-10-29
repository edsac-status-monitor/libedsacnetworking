/*
 * Copyright 2017
 * GPL3 Licensed
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
#include <time.h>
#include <stdio.h>
#include <assert.h>

/* So what is going on here?
The server uses signal driven IO so that it is not constantly polling dosens of clients.
We use two signal handlers: CONNECT_SIG for when a new client connects and READ_SIG for when new IO is available on a socket

The handler for READ_SIG is quite complex. It can only access shared resources asynchronously by spawning worker threads incase we are interupting code
already has the control mutex locked (in which case a deadlock would occur if we operated synchronously).

When a message is read in it is added to the read_buff queue. Items are requested and returned from this queue at some later time using read_message()

Also, clients are expected to periodically send KEEP_ALIVE messages so that we know that they are running. The time of the most recent one of these is stored in the connection table.
Periodically these times are checked against the current time to see if everything is it should be. This is done on the signal handler for SIGALRM.
*/

// ofsets past REALTIMESIGMIN
#define CONNECT_SIG 1
#define READ_SIG 2

// global read buffer
static GQueue *read_buff = NULL;
static pthread_mutex_t read_buff_mux = PTHREAD_MUTEX_INITIALIZER;

// global store of connections
static pthread_mutex_t connections_mux = PTHREAD_MUTEX_INITIALIZER;
static GHashTable *connections_table = NULL;

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

// attempt to read a full json object from a fd (defined as "{*}", handling nesting)
static ReadStatus read_json_object(int fd, GString **out_string) {
    // assuming that we can read the whole thing at once

    GString *string = g_string_new("{");

    // read first character
    char buf;
    ssize_t num_read = read(fd, &buf, 1);
    if (1 != num_read) { // we could not read a whole character
        int e = errno;
        g_string_free(string, true);

        // if it looks like there just wasn't any data left in the buffer for us to read
        if ((EBADF == e) || (EAGAIN == e) || (EWOULDBLOCK == e) || (0 == num_read)) {
            return END;
        }
        
        // there may have been data but an unknown error occured
        printf("Unknown error errno=%i num_read=%li\n", e, num_read);
        return ERROR;
    }

    // the first character must be {
    if (buf != '{') {
        g_string_free(string, true);

        // skip newline characters so we can telnet in for testing
        if ((buf == '\n') || (buf == 13 /*CR*/)) { 
            puts("newline");
            return read_json_object(fd, out_string);
        }

        printf("invalid c=%i\n", (int) buf);
        return ERROR;
    }

    // read in the rest
    int nest_count = 1; // we already have {
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

        // we did read in a character successfully...
        
        // nesting count
        if ('{' == buf)
            nest_count += 1;
        else if ('}' == buf)
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
// mutexes are done by the caller
static ReadStatus fetch_item(ConnectionData *condata) {
    // the item we will add to the buffer for this read
    BufferItem *item = malloc(sizeof(BufferItem));
    if (NULL == item) {
        return ERROR;
    }

    // attempt to read in the json object
    GString *obj;
    ReadStatus status = read_json_object(condata->fd, &obj);
    if (SUCCESS != status) {
        free(item);
        return status;
    }

    // decode JSON
    if (!decode_message(obj->str, &(item->msg))) {
        puts("decode error\n");
        // report this BufferItem as a software error
        software_error(&(item->msg), "Could not decode message");
    }

    g_string_free(obj, true);

    if (KEEP_ALIVE == item->msg.type) {
        free_bufferitem(item);
        // update last_keep_alive
        if (-1 == time(&(condata->last_keep_alive))) {
            puts("couldn't get time");
            return ERROR;
        }
    } else { // "real" messages
        item->address = condata->addr.sin_addr;

        // add the item to the queue
        g_queue_push_tail(read_buff, (gpointer) item);
    }

    return SUCCESS;
}

static void destroy_connection(ConnectionData *condata) {
    // destroy the connection

    // get access to connections_table
    if (0 != pthread_mutex_lock(&connections_mux)) {
        puts("Couldn't remove item from hash table");
        return;
    }
    
    // calls free_connectiondata for us
    if (TRUE != g_hash_table_remove(connections_table, &(condata->fd))) {
        puts("Couldn't remove item from hash table");
        pthread_mutex_unlock(&connections_mux);
    }

    pthread_mutex_unlock(&connections_mux);
}

// thread to read in all of the objects waiting in a buffer
/* the actual reading is done in its own thread for the following situation:
 *    1. some normal code has read_buff_mux locked
 *    2. io_handler signal tries to lock read_buff_mux
 *    3. deadlock
 *
 * Using s different thread for the io_handler means that it can block waiting for read_buff_mux without creating a deadlock
 * We handle mutexes out here to force the program to behave more serially:
 *    For example, if the read queue was only locked while it was being used, the system test becomes a race to see if this thread can finish
 *      adding to the queue before the queue item is requested by the test. This way the queue item request can't jump in so easily
 *      (theoretically it could jump in before this thread has had any chance to lock the mutex but in practice this seems not to happen. 
 *      I can't see a better solution than this because we can't lock the mutex in the signal handler because of the above deadlock)
 */
static void *object_reader_thread(void *arg) {
    ConnectionData *condata = (ConnectionData *) arg;
    
    // get exclusive access to read_buff
    if (0 != pthread_mutex_lock(&read_buff_mux)) {
        puts("object reader could not get the read_buff mutex");
        free(arg);
        return NULL;
    }
    
    // read in every available object
    while (true) {
        ReadStatus status = fetch_item(condata);
        if (ERROR == status) {
            puts("Read ERROR from remote host\n"); 
            destroy_connection(condata);
            pthread_mutex_unlock(&read_buff_mux);
            return NULL;
        } else if (END == status) { // we have read the whole buffer
            break;
        }
        // else we had a SUCCESS so continue (the buffer may contain another message)
    }
    
    // clean up before returning
    pthread_mutex_unlock(&read_buff_mux);
    pthread_mutex_unlock(&(condata->mutex));
    return NULL;
}

// another thread for reporting a connection close
static void *report_close_thread(void *arg) {
    ConnectionData *condata = (ConnectionData *) arg;
    
    // we are done with the fd now
    pthread_mutex_unlock(&(condata->mutex));
    
    // allocate the item to go onto the queue
    BufferItem *item = malloc(sizeof(BufferItem));
    if (!item) {
        return NULL;
    }
    
    item->address = condata->addr.sin_addr;
    
    software_error(&(item->msg), "Connection closed");
    
    // get access to the queue
    if (0 != pthread_mutex_lock(&read_buff_mux)) {
        puts("can't lock queue");
        free_bufferitem(item);
        return NULL;
    }
    
    // add the item to the queue
    g_queue_push_tail(read_buff, (gpointer) item);
    
    pthread_mutex_unlock(&read_buff_mux);
    puts("connection close added to queue");

    destroy_connection(condata);
    return NULL;
}

// realtime signal handler for when IO is available on a connection with a client
static void io_handler(__attribute__ ((unused)) int sig, __attribute__ ((unused)) siginfo_t *si, __attribute__ ((unused)) void *ucontext) {
    pthread_t thread;

    // look up the file descriptor in the connections table
    if (0 != pthread_mutex_lock(&connections_mux)) {
        puts("couldn't lock connections table");
        return;
    }

    ConnectionData *condata = g_hash_table_lookup(connections_table, &(si->si_fd));
    pthread_mutex_unlock(&connections_mux);
    if (NULL == condata) {
        printf("%i not in table\n", si->si_fd);
        return;
    }
    assert(si->si_fd == condata->fd);

    // get the exclusive right to do reading as early as we can incase another thread closes the file descriptor
    if (0 != pthread_mutex_lock(&(condata->mutex))) {
        puts("can't lock reading mux");
        return;
    }

    char buf;
    // check to see if there is any data to read
    // checking si->si_code just seems to report the connection is closed on every signal
    // MSG_PEEK so that the read character is not removed from the read buffer
    ssize_t count = recv(si->si_fd, &buf, 1, MSG_PEEK);
    int e = errno;
    if (1 != count) {
        // BUG: this may report the connection close more than once. si_code, errno and count seem the same each time so I don't know how to tell
        printf("closed connection. fd=%i si_code=%i errno=%i count=%li\n",si->si_fd, si->si_code, e, count);

        // start thread
        pthread_create(&thread, NULL, report_close_thread, (void *) condata); // unlocks the reading mutex
        return;
    }

    // do the actual reading in a different thread (see comment on thread function)
    pthread_create(&thread, NULL, object_reader_thread, (void *) condata); // this thread unlocks the reading mux when it is done
}

// realtime signal handler for when there is a connection to the listening socket
static void connect_handler(__attribute__ ((unused)) int sig, siginfo_t *si, __attribute__ ((unused)) void *ucontext) {
    // accepts a connection from a remote host and sets it up to do signal driven IO
    int fd = accept(si->si_fd, NULL, NULL);
    if (-1 == fd)
        return;

    // add fd to the connections table:

    // allocate memory for the ConnectionData
    ConnectionData *condata = malloc(sizeof(ConnectionData));
    if (NULL == condata) {
        close(fd);
        return;
    }

    // get the remote address
    socklen_t addrlen = sizeof(condata->addr);
    if (0 != getpeername(fd, &(condata->addr), &addrlen)) {
        close(fd);
        free(condata);
        return;
    }
    // I found experimentally that getpeername only works on the second call. Tested on Ubuntu, Debian and CentOS
    if (0 != getpeername(fd, &(condata->addr), &addrlen)) {
        close(fd);
        free(condata);
        return;
    }
    char addr[160] = {'\n'}; // buffer to hold string-ified ip4 address
    inet_ntop(AF_INET, &(condata->addr.sin_addr.s_addr), addr, sizeof(addr));
    printf("Connect from %s\n", addr);

    // set up condata->mutex
    if (-1 == pthread_mutex_init(&(condata->mutex), NULL)) {
        close(fd);
        free(condata);
        return;
    }

    // set the last message time to now
    if (-1 == time(&(condata->last_keep_alive))) {
        close(fd);
        free_connectiondata(condata);
        return;
    }

    condata->fd = fd;

    // get access to connections table
    if (0 != pthread_mutex_lock(&connections_mux)) {
        close(fd);
        free_connectiondata(condata);
        return;
    }

    // g_hash_table needs the key to stick around
    int *key = malloc(sizeof(int));
    if (NULL == key) {
        close(fd);
        free_connectiondata(condata);
        return;
    }
    *key = fd;

    // put it into the connections table
    if (FALSE == g_hash_table_insert(connections_table, key, condata)) {
        puts("ERROR: duplicate entry in connections table!");
        exit(EXIT_FAILURE);
        return;
    }

    pthread_mutex_unlock(&connections_mux);

    printf("%i in table\n", fd);

    setup_rt_signal_io(fd, SIGRTMIN + READ_SIG, io_handler);
}

// function to check if a keep alive message has been received for a given connection
static void check_keep_alive(__attribute__((unused)) gpointer key, gpointer value, __attribute__((unused)) gpointer user_data) {
    if (NULL == value)
        return;

    ConnectionData *condata = (ConnectionData *) value;

    // get current time
    time_t now = time(NULL);
    if (-1 == now)
        return;

    // calculate time difference
    time_t diff = now - condata->last_keep_alive;

    if (diff > (KEEP_ALIVE_PROD)) {
        char addr[16] = {'\n'}; // buffer to hold string-ified ip4 address
        inet_ntop(AF_INET, &(condata->addr.sin_addr.s_addr), addr, sizeof(addr));
        printf("No KEEP_ALIVE from %s (fd=%i) for %li seconds!\n", addr, condata->fd, diff);

        // report error and close the connection
        BufferItem *err = malloc(sizeof(BufferItem));
        if (NULL == err)
            return;
        software_error(&(err->msg), "Connection timeout");
        memcpy(&(err->address), &(condata->addr), sizeof(err->address));

        if (0 != pthread_mutex_trylock(&read_buff_mux)) {
            free_bufferitem(err);
            return;
        }

        g_queue_push_tail(read_buff, err);
        pthread_mutex_unlock(&read_buff_mux);
        puts("error in queue");
    }
}

// called periodically to check if we have received a KEEP_ALIVE message recently
static void iter_keep_alives(__attribute__((unused)) int compulsory) {
    // schedule the next run of this
    alarm((KEEP_ALIVE_INTERVAL) * (KEEP_ALIVE_CHECK_PERIOD));

    // get lock on connections_table
    // only doing trylock because it doesn't matter if we skip this every so often
    if (0 != pthread_mutex_trylock(&connections_mux)) {
        return;
    }

    // check each connection
    g_hash_table_foreach(connections_table, check_keep_alive, NULL);

    pthread_mutex_unlock(&connections_mux);
}


// starts a server listening on addr
// returns success
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

    // initialise the read buffer
    read_buff = g_queue_new();
    if (!read_buff) {
        close(listen_socket);
        listen_socket = -1;
        return false;
    }

    // initialise the connections table
    connections_table = g_hash_table_new_full(g_int_hash, g_int_equal, (GDestroyNotify) free, (GDestroyNotify) free_connectiondata); 
    if (!connections_table) {
        close(listen_socket);
        listen_socket = -1;
        return false;
    }

    // set up realtime signal-driven IO on the listening socket
    if (!setup_rt_signal_io(listen_socket, SIGRTMIN + CONNECT_SIG, connect_handler)) {
        close(listen_socket);
        listen_socket = -1;
        g_queue_free(read_buff);
        read_buff = NULL;
        g_hash_table_destroy(connections_table);
        connections_table = NULL;
        return false;
    }

    // set up keep_alive checker
    if (SIG_ERR == signal(SIGALRM, iter_keep_alives)) {
        close(listen_socket);
        listen_socket = -1;
        g_queue_free(read_buff);
        read_buff = NULL;
        g_hash_table_destroy(connections_table);
        connections_table = NULL;
        return false;
    }
    alarm((KEEP_ALIVE_INTERVAL) * (KEEP_ALIVE_CHECK_PERIOD));

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

// gets a message from the read queue
BufferItem *read_message(void) {
    if (0 != pthread_mutex_lock(&read_buff_mux))
        return NULL;

    BufferItem *ret = (BufferItem *) g_queue_pop_head(read_buff);
    // if this is NULL we should be returning NULL anyway

    pthread_mutex_unlock(&read_buff_mux);

    return ret;
}

// free a BufferItem (wrapper function incase it contains anyting that needs freeing interneally)
void free_bufferitem(BufferItem *item) {
    free_message(&(item->msg));
    free(item);
}

// free a ConnectionData
void free_connectiondata(ConnectionData *condata) {
    pthread_mutex_destroy(&(condata->mutex));
    close(condata->fd);
    free(condata);
}

void stop_server(void) {
    // free up the connection table and close all the active connections
    if (connections_table) {
        pthread_mutex_lock(&connections_mux);
        g_hash_table_destroy(connections_table);
        connections_table = NULL;
        pthread_mutex_unlock(&connections_mux);
    }
    
    puts("about to close listen socket");
    // close the open socket
    if (-1 != listen_socket) {
        close(listen_socket);
        listen_socket = -1;
    }
    puts("listen socket closed");
    
    // disable signal handlers
    struct sigaction sa;
    DISABLE_SIGNAL(SIGRTMIN + READ_SIG)
    DISABLE_SIGNAL(SIGALRM)
    DISABLE_SIGNAL(SIGRTMIN + CONNECT_SIG)

    // free up the read buffer
    if (read_buff) {
        pthread_mutex_lock(&read_buff_mux);
        g_queue_free_full(read_buff, (GDestroyNotify) free_bufferitem);
        read_buff = NULL;
        pthread_mutex_unlock(&read_buff_mux);
    }

}