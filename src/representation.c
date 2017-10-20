/*
 * Copyright 2017
 * MIT Licenced
 * representation.c
 * This is just a temporary hello world program to test that the build system is working. Also tries using cJSON.
 */

#include "config.h"
#include "representation.h"
#include <stdio.h>
#include <assert.h>
#include "contrib/cJSON.h"
#include <string.h>

void hardware_error(ErrorMessage *message, int valve_no, int test_point_no, bool test_point_high) {
    if (NULL == message)
        return;

    message->type = HARD_ERROR;
    message->data.hardware.valve_no = valve_no;
    message->data.hardware.test_point_no = test_point_no;
    message->data.hardware.test_point_high = test_point_high;
}

void software_error(ErrorMessage *message, const char* string) {
    if (NULL == message)
        return;

    message->type = SOFT_ERROR;
    message->data.software.message = string;
}

// shorthand
#define NULL_CHECK(ptr, root) \
    if (NULL == ptr) { \
        cJSON_Delete(root); \
        return -1; \
    }

ssize_t encode_message(const ErrorMessage *message, char **encoded_message) {
    // arguments check
    if ((NULL == message) || (NULL == encoded_message))
        return -1;

    // all message types contain a version field
    cJSON *version_number = cJSON_CreateNumber(DATA_FORMAT_VERSION);
    if (NULL == version_number)
        return -1;

    // create a root and add the version number to it
    cJSON *root = cJSON_CreateObject();
    NULL_CHECK(root, version_number)
    cJSON_AddItemToObject(root, "version", version_number);

    // data subroot
    cJSON *data = cJSON_CreateObject();
    NULL_CHECK(data, root)
    cJSON_AddItemToObject(root, "data", data);

    // the rest depends on the message type
    switch (message->type) {
        case HARD_ERROR: ; // hardware error
            cJSON *hard_err_type = cJSON_CreateString("HARD_ERROR");
            NULL_CHECK(hard_err_type, root)
            cJSON_AddItemToObject(root, "type", hard_err_type);

            // valve_no
            cJSON *valve_no = cJSON_CreateNumber((double) message->data.hardware.valve_no);
            NULL_CHECK(valve_no, root)
            cJSON_AddItemToObject(data, "valve_no", valve_no);

            // test_point_no
            cJSON *test_point_no = cJSON_CreateNumber((double) message->data.hardware.test_point_no);
            NULL_CHECK(test_point_no, root)
            cJSON_AddItemToObject(data, "test_point_no", test_point_no);

            // test_point_high
            cJSON *test_point_high = cJSON_CreateBool(message->data.hardware.test_point_high);
            NULL_CHECK(test_point_high, root)
            cJSON_AddItemToObject(data, "test_point_high", test_point_high);
            break;

        case SOFT_ERROR: ;
            cJSON *soft_err_type = cJSON_CreateString("SOFT_ERROR");
            NULL_CHECK(soft_err_type, root)
            cJSON_AddItemToObject(root, "type", soft_err_type);

            // message
            cJSON *cjson_message = cJSON_CreateString(message->data.software.message);
            NULL_CHECK(cjson_message, root)
            cJSON_AddItemToObject(data, "message", cjson_message);
            break;

        case INVALID: // invalid message
        default: // anything else
            cJSON_Delete(root); 
            return -1;           
    }

    *encoded_message = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (NULL == *encoded_message) {
        return -1;
    }

    // work out the encoded message length
    ssize_t len = (ssize_t) strnlen(*encoded_message, MAX_ENCODED_LEN);
    
    return len;
}
