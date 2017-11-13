/*
 * Copyright 2017
 * GPL3 License
 * representation.h
 * Functions relating to the wire representation of data
 */

#ifndef REPRES_H
#define REPRES_H

// link properly with C++
#ifdef _cplusplus
extern "C" {
#endif // _cplusplus

// includes
#include <stdbool.h>
#include <sys/types.h> // ssize_t size_t 
#include <stdint.h>
#include <arpa/inet.h>
#include <glib.h>

// declarations

// the type of message sent
typedef enum {
    HARD_ERROR_VALVE, // the monitor found a hardware error down to a particular valve
    HARD_ERROR_OTHER, // the monitor found a hardware error which cannot be narrowed down to one valve
    SOFT_ERROR,       // the monitor encountered a software error
    KEEP_ALIVE,       // message from the client to the server to show it is still there
    INVALID,          // invalid message type
} MessageType;

// hardware error valve message
typedef struct {
    int valve_no;         // the number of the valve expected to be broken
    GString *message;     // a description of the error
} HardErrorValveData;

#define MAX_MSG_LEN 200 // maximum allowable length for SoftErrorData.message and HardErrorOtherData.message

// hardware error other message
typedef struct {
    GString *message; // message to be reported in the UI
} HardErrorOtherData;

// software error message
typedef struct {
    GString *message; // message to be reported to the UI
} SoftErrorData;

// combined representation of the data sections
typedef union {
    HardErrorValveData hardware_valve;
    HardErrorOtherData hardware_other;
    SoftErrorData software;
} MessageData;

// representation of the full message
typedef struct {
    MessageType type;
    MessageData data;
} Message;

// sets up an ip v4 address structure
// returns NULL on failure. addr is a string representation e.g. "127.0.0.1"
// the returned address should be free()'ed after use
struct sockaddr *alloc_addr(const char *addr, uint16_t port);

// function to initialise a hardware error valve structure
void hardware_error_valve(Message *message, int valve_no, const char *string);

// function to initialise a hardware error other structure
void hardware_error_other(Message *message, const char *string);

// function to initialise a software error structure
void software_error(Message *message, const char *string);

// function to initialise a keep alive message
void keep_alive(Message *message);

// function to encode a message. Dynamically allocates storage
// returns the size of the encoded message or -1 on error
ssize_t encode_message(const Message *message, char **encoded_message);

// function to decode a message. 
// Returns success or failure
bool decode_message(const char* encoded_message, Message *message);

// frees dynamically allocated memory *within* a message (aka this will not free the message structure itself)
void free_message(Message *msg);

// internals
#define DATA_FORMAT_VERSION 2.0
#define MAX_ENCODED_LEN ((MAX_MSG_LEN) + 100) // approximate

#ifdef _cplusplus
}
#endif // _cplusplus
#endif // REPRES_H