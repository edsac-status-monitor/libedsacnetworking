/*
 * Copyright 2017
 * MIT Licenced
 * representation.c
 * Functions relating to the representation of network messages
 */

#include "config.h"
#include "representation.h"
#include <stdio.h>
#include <assert.h>
#include "contrib/cJSON.h"
#include <string.h>
#include <stdlib.h>

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

// initialises a hardware error message
void hardware_error(ErrorMessage *message, int valve_no, int test_point_no, bool test_point_high) {
    if (NULL == message)
        return;

    message->type = HARD_ERROR;
    message->data.hardware.valve_no = valve_no;
    message->data.hardware.test_point_no = test_point_no;
    message->data.hardware.test_point_high = test_point_high;
}

// initialises a software error message
void software_error(ErrorMessage *message, const char *string) {
    if (NULL == message)
        return;

    message->type = SOFT_ERROR;
    message->data.software.message = (char *) string;
}

// shorthand to bail if a pointer is NULL
#define NULL_CHECK(_ptr, _root, _ret_val) \
    if (NULL == _ptr) { \
        cJSON_Delete(_root); \
        return _ret_val; \
    }

// encode a message structure into a format to be transmitted
// returns the length of the encoded_message string
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
    NULL_CHECK(root, version_number, -1)
    cJSON_AddItemToObject(root, "version", version_number);

    // data subroot
    cJSON *data = cJSON_CreateObject();
    NULL_CHECK(data, root, -1)
    cJSON_AddItemToObject(root, "data", data);

    // the rest depends on the message type
    switch (message->type) {
        // hardware error
        case HARD_ERROR: ;  // ; is because C won't define a variable as the first statement after a goto
            // message type
            cJSON *hard_err_type = cJSON_CreateString("HARD_ERROR");
            NULL_CHECK(hard_err_type, root, -1)
            cJSON_AddItemToObject(root, "type", hard_err_type);

            // valve_no
            cJSON *valve_no = cJSON_CreateNumber((double) message->data.hardware.valve_no);
            NULL_CHECK(valve_no, root, -1)
            cJSON_AddItemToObject(data, "valve_no", valve_no);

            // test_point_no
            cJSON *test_point_no = cJSON_CreateNumber((double) message->data.hardware.test_point_no);
            NULL_CHECK(test_point_no, root, -1)
            cJSON_AddItemToObject(data, "test_point_no", test_point_no);

            // test_point_high
            cJSON *test_point_high = cJSON_CreateBool(message->data.hardware.test_point_high);
            NULL_CHECK(test_point_high, root, -1)
            cJSON_AddItemToObject(data, "test_point_high", test_point_high);
            break;

        case SOFT_ERROR: ;
            // message type
            cJSON *soft_err_type = cJSON_CreateString("SOFT_ERROR");
            NULL_CHECK(soft_err_type, root, -1)
            cJSON_AddItemToObject(root, "type", soft_err_type);

            // message
            cJSON *cjson_message = cJSON_CreateString(message->data.software.message);
            NULL_CHECK(cjson_message, root, -1)
            cJSON_AddItemToObject(data, "message", cjson_message);
            break;

        case INVALID: // invalid message
        default: // or anything else
            cJSON_Delete(root); 
            return -1;           
    }

    // encode JSON as string
    *encoded_message = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (NULL == *encoded_message) {
        return -1;
    }

    // work out the encoded message length
    ssize_t len = (ssize_t) strnlen(*encoded_message, MAX_ENCODED_LEN);
    
    return len;
}

// shorthand to check the type of a node in the cJSON tree
#define EXPECT_TYPE(_ptr, _type) \
    if (!cJSON_Is##_type(_ptr)) { \
        cJSON_Delete(root); \
        return false; \
    } \

// decode a string into a message structure
// returns success
bool decode_message(const char* encoded_message, ErrorMessage *message) {
    // arguments check
    if ((NULL == encoded_message) || (NULL == message))
        return false;

    // parse JSON
    cJSON *root = cJSON_Parse(encoded_message);
    if (NULL == root)
        return false;

    // check the version number
    cJSON *version = cJSON_GetObjectItem(root, "version");
    NULL_CHECK(version, root, false)
    EXPECT_TYPE(version, Number)
    if (DATA_FORMAT_VERSION != version->valuedouble) {
        cJSON_Delete(root);
        return false;
    }

    // data subroot
    cJSON *data = cJSON_GetObjectItem(root, "data");
    NULL_CHECK(data, root, false)

    // message type
    cJSON *type = cJSON_GetObjectItem(root, "type");
    NULL_CHECK(type, root, false)
    EXPECT_TYPE(type, String)

    if (0 == strncmp("SOFT_ERROR", type->valuestring, 20)) {
        // it was a software error packet 

        // get description string
        cJSON *description = cJSON_GetObjectItem(data, "message");
        NULL_CHECK(description, root, false)
        EXPECT_TYPE(description, String)

        // make a copy of the string so it isn't lost on cJSON_Delete
        size_t len = strnlen(description->valuestring, MAX_MSG_LEN) + 1;
        char *desc = (char *) malloc(len);
        NULL_CHECK(desc, root, false)
        strncpy(desc, description->valuestring, len);

        // initialise message
        software_error(message, desc);
    
    } else if (0 == strncmp("HARD_ERROR", type->valuestring, 20)) {
        // it was a hardware error packet

        // valve_no
        cJSON *valve_no = cJSON_GetObjectItem(data, "valve_no");
        NULL_CHECK(valve_no, root, false)
        EXPECT_TYPE(valve_no, Number)

        // test_point_no
        cJSON *test_point_no = cJSON_GetObjectItem(data, "test_point_no");
        NULL_CHECK(test_point_no, root, false)
        EXPECT_TYPE(test_point_no, Number)

        // test_point_high
        cJSON *test_point_high = cJSON_GetObjectItem(data, "test_point_high");
        NULL_CHECK(test_point_high, root, false)
        EXPECT_TYPE(test_point_high, Bool)

        // initialize message
        hardware_error(message, valve_no->valueint, test_point_no->valueint, test_point_high->type == cJSON_True);

    } else {
        // we don't know what kind of packet that is
        cJSON_Delete(root);
        return false;
    }
    
    cJSON_Delete(root);
    return true;
}
