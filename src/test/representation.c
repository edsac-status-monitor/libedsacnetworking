/*
 * Copyright 2017
 * MIT Licenced
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
    ErrorMessage hardware;
    hardware_error(&hardware, 1, 2, true);

    ErrorMessage software;
    software_error(&software, "blah blah software broke");

    ErrorMessage invalid;
    invalid.type = INVALID;

    char *msg;
    const char *hardware_expected = "{\"version\":1,\"data\":{\"valve_no\":1,\"test_point_no\":2,\"test_point_high\":true},\"type\":\"HARD_ERROR\"}";
    const char *software_expected = "{\"version\":1,\"data\":{\"message\":\"blah blah software broke\"},\"type\":\"SOFT_ERROR\"}";

    // test encoding a HARD_ERROR message
    assert(-1 != encode_message(&hardware, &msg));
    //printf("%s\n", msg);
    assert(0 == strncmp(hardware_expected, msg, MAX_ENCODED_LEN));
    free(msg);
    msg = NULL;

    // test encoding a SOFT_ERROR message
    assert(-1 != encode_message(&software, &msg));
    //printf("%s\n", msg);
    assert(0 == strncmp(software_expected, msg, MAX_ENCODED_LEN));
    free(msg);
    msg = NULL;

    // test that we cannot encode an invalid message
    assert(-1 == encode_message(&invalid, &msg));
}

static void test_decoding(void) {
    // some encoded messages to play with
    const char *invalid_json = "{blah";
    const char *invalid_data = "{\"unrelated\":3.14159}";
    const char *invalid_version = "{\"version\":-1}";
    const char *hardware = "{\"version\":1,\"data\":{\"valve_no\":2,\"test_point_no\":3,\"test_point_high\":false},\"type\":\"HARD_ERROR\"}";
    const char *software = "{\"version\":1,\"data\":{\"message\":\"hello world!\"},\"type\":\"SOFT_ERROR\"}";

    // memory to put stuff in
    ErrorMessage msg;

    // test invalid cases
    assert(!decode_message(invalid_json, &msg));
    assert(!decode_message(invalid_data, &msg));
    assert(!decode_message(invalid_version, &msg));

    // software packet
    assert(decode_message(software, &msg));
    assert(SOFT_ERROR == msg.type);
    assert(0 == strncmp("hello world!", msg.data.software.message, 13));
    free(msg.data.software.message);

    // hardware packet
    assert(decode_message(hardware, &msg));
    assert(HARD_ERROR == msg.type);
    assert(2 == msg.data.hardware.valve_no);
    assert(3 == msg.data.hardware.test_point_no);
    assert(false == msg.data.hardware.test_point_high);
}

int main(void) {
    test_encoding();
    test_decoding();

    return EXIT_SUCCESS;
}
