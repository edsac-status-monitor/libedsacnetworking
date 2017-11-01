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


static void test_encoding(void) {
    Message hardware_valve;
    hardware_error_valve(&hardware_valve, 1, 2, true);

    Message hardware_other;
    hardware_error_other(&hardware_other, "blah blah hardware broke");

    Message software;
    software_error(&software, "blah blah software broke");

    Message keep_alive_msg;
    keep_alive(&keep_alive_msg);

    Message invalid;
    invalid.type = INVALID;

    char *msg;
    const char *hardware_valve_expected = "{\"version\":1,\"data\":{\"valve_no\":1,\"test_point_no\":2,\"test_point_high\":true},\"type\":\"HARD_ERROR_VALVE\"}";
    const char *hardware_other_expected = "{\"version\":1,\"data\":{\"message\":\"blah blah hardware broke\"},\"type\":\"HARD_ERROR_OTHER\"}";
    const char *software_expected = "{\"version\":1,\"data\":{\"message\":\"blah blah software broke\"},\"type\":\"SOFT_ERROR\"}";
    const char *keep_alive_expected = "{\"version\":1,\"data\":{},\"type\":\"KEEP_ALIVE\"}";

    // test encoding a HARD_ERROR_VALVE message
    assert(-1 != encode_message(&hardware_valve, &msg));
    assert(0 == strncmp(hardware_valve_expected, msg, MAX_ENCODED_LEN));
    free(msg);
    msg = NULL;

    // test encoding a HARD_ERROR_OTHER message
    assert(-1 != encode_message(&hardware_other, &msg));
    assert(0 == strncmp(hardware_other_expected, msg, MAX_ENCODED_LEN));
    free(msg);
    msg = NULL;

    // test encoding a SOFT_ERROR message
    assert(-1 != encode_message(&software, &msg));
    assert(0 == strncmp(software_expected, msg, MAX_ENCODED_LEN));
    free(msg);
    msg = NULL;

    // test encoding a KEEP_ALIVE message
    assert(-1 != encode_message(&keep_alive_msg, &msg));
    assert(0 == strncmp(keep_alive_expected, msg, MAX_ENCODED_LEN));
    free(msg);
    msg = NULL;

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

static void test_decoding(void) {
    // some encoded messages to play with
    const char *invalid_json = "{blah";
    const char *invalid_data = "{\"unrelated\":3.14159}";
    const char *invalid_version = "{\"version\":-1}";
    const char *hardware_valve = "{\"version\":1,\"data\":{\"valve_no\":2,\"test_point_no\":3,\"test_point_high\":false},\"type\":\"HARD_ERROR_VALVE\"}";
    const char *hardware_other = "{\"version\":1,\"data\":{\"message\":\"foo bar\"},\"type\":\"HARD_ERROR_OTHER\"}";
    const char *software = "{\"version\":1,\"data\":{\"message\":\"hello world!\"},\"type\":\"SOFT_ERROR\"}";
    const char *keep_alive_encoded = "{\"version\":1,\"data\":{},\"type\":\"KEEP_ALIVE\"}";

    // memory to put stuff in
    Message msg;

    // test invalid cases
    assert(!decode_message(invalid_json, &msg));
    assert(!decode_message(invalid_data, &msg));
    assert(!decode_message(invalid_version, &msg));

    // software packet
    assert(decode_message(software, &msg));
    assert(SOFT_ERROR == msg.type);
    assert(0 == strncmp("hello world!", msg.data.software.message->str, 13));
    free_message(&msg);

    // hardware valve packet
    assert(decode_message(hardware_valve, &msg));
    assert(HARD_ERROR_VALVE == msg.type);
    assert(2 == msg.data.hardware_valve.valve_no);
    assert(3 == msg.data.hardware_valve.test_point_no);
    assert(false == msg.data.hardware_valve.test_point_high);
    free_message(&msg);

    // hardware other packet
    assert(decode_message(hardware_other, &msg));
    assert(HARD_ERROR_OTHER == msg.type);
    assert(0 == strncmp("foo bar", msg.data.hardware_other.message->str, 8));
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
