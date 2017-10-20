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
    printf("%s\n", msg);
    assert(0 == strncmp(hardware_expected, msg, MAX_ENCODED_LEN));
    free(msg);
    msg = NULL;

    // test encoding a SOFT_ERROR message
    assert(-1 != encode_message(&software, &msg));
    printf("%s\n", msg);
    assert(0 == strncmp(software_expected, msg, MAX_ENCODED_LEN));
    free(msg);
    msg = NULL;

    // test that we cannot encode an invalid message
    assert(-1 == encode_message(&invalid, &msg));
}

int main(void) {
    test_encoding();

    return EXIT_SUCCESS;
}
