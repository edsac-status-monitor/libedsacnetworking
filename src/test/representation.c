/*
 * Copyright 2017
 * GPL3 Licensed 
 * test/representation.c
 * Testsuite for representation.c
 */

#include "config.h"
#include "representation.h"
#include <stdlib.h> // EXIT_*
#include <stdio.h>
#include <assert.h>
#include <string.h>

// shorthand for similar message types
#define MESSAGE_ENCODE(_message) \
    assert(-1 != encode_message(&_message, &msg)); \
    assert(0 == strncmp(_message##_expected, msg, MAX_ENCODED_LEN)); \
    free(msg); \
    msg = NULL;

static void test_encoding(void) {
    Message hardware_valve;
    hardware_error_valve(&hardware_valve, 1, "blah blah valve broke");

    Message hardware_other;
    hardware_error_other(&hardware_other, "blah blah hardware broke");

    Message software;
    software_error(&software, "blah blah software broke");

    Message keep_alive_msg;
    keep_alive(&keep_alive_msg);

    Message invalid;
    invalid.type = INVALID;

    char *msg;
    const char *hardware_valve_expected = "{\"version\":2,\"data\":{\"message\":\"blah blah valve broke\",\"valve_no\":1},\"type\":\"HARD_ERROR_VALVE\"}";
    const char *hardware_other_expected = "{\"version\":2,\"data\":{\"message\":\"blah blah hardware broke\"},\"type\":\"HARD_ERROR_OTHER\"}";
    const char *software_expected = "{\"version\":2,\"data\":{\"message\":\"blah blah software broke\"},\"type\":\"SOFT_ERROR\"}";
    const char *keep_alive_msg_expected = "{\"version\":2,\"data\":{},\"type\":\"KEEP_ALIVE\"}";

    // test encoding a HARD_ERROR_VALVE message
    MESSAGE_ENCODE(hardware_valve)

    // test encoding a HARD_ERROR_OTHER message
    MESSAGE_ENCODE(hardware_other)

    // test encoding a SOFT_ERROR message
    MESSAGE_ENCODE(software)

    // test encoding a KEEP_ALIVE message
    MESSAGE_ENCODE(keep_alive_msg)

    // test that we cannot encode an invalid message
    assert(-1 == encode_message(&invalid, &msg));
    free(msg);
    msg = NULL;

    // free message contents (only strictly necessary for the software message)
    free_message(&software);
    free_message(&hardware_valve);
    free_message(&hardware_other);
    free_message(&keep_alive_msg);
    free_message(&invalid);
}

// shorthand for similar message types
#define PRINTABLE_MESSAGE_DECODE(_message_type, _type, _str) \
    assert(decode_message(_message_type, &msg)); \
    assert(_type == msg.type); \
    assert(0 == strncmp(_str, msg.data._message_type.message->str, strlen(_str)));

static void test_decoding(void) {
    // some encoded messages to play with
    const char *invalid_json = "{blah";
    const char *invalid_data = "{\"unrelated\":3.14159}";
    const char *invalid_version = "{\"version\":-1}";
    const char *hardware_valve = "{\"version\":2,\"data\":{\"valve_no\":2,\"message\":\"my string\"},\"type\":\"HARD_ERROR_VALVE\"}";
    const char *hardware_other = "{\"version\":2,\"data\":{\"message\":\"foo bar\"},\"type\":\"HARD_ERROR_OTHER\"}";
    const char *software = "{\"version\":2,\"data\":{\"message\":\"hello world!\"},\"type\":\"SOFT_ERROR\"}";
    const char *keep_alive_encoded = "{\"version\":2,\"data\":{},\"type\":\"KEEP_ALIVE\"}";

    // memory to put stuff in
    Message msg;

    // test invalid cases
    assert(!decode_message(invalid_json, &msg));
    assert(!decode_message(invalid_data, &msg));
    assert(!decode_message(invalid_version, &msg));

    // software packet
    PRINTABLE_MESSAGE_DECODE(software, SOFT_ERROR, "hello world!")
    free_message(&msg);

    // hardware valve packet
    PRINTABLE_MESSAGE_DECODE(hardware_valve, HARD_ERROR_VALVE, "my string")
    assert(2 == msg.data.hardware_valve.valve_no);
    free_message(&msg);

    // hardware other packet
    PRINTABLE_MESSAGE_DECODE(hardware_other, HARD_ERROR_OTHER, "foo bar")
    free_message(&msg);

    // keep alive
    assert(decode_message(keep_alive_encoded, &msg));
    assert(KEEP_ALIVE == msg.type);
    free_message(&msg);
}

int main(void) {
    test_encoding();
    test_decoding();

    return EXIT_SUCCESS;
}
