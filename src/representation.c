/*
 * Copyright 2017
 * GPL3 Licensed
 * representation.c
 * Functions relating to the representation of network messages
 */

#include "config.h"
#include "edsac_representation.h"
#include <stdio.h>
#include <assert.h>
#include "contrib/cJSON.h"
#include <string.h>
#include <stdlib.h>

// shorthand for the initialisation functions
#define PRINTABLE_MSG(_message, _string, _data_type, _type) \
    if (NULL == _message){ \
        return; \
    } \
    _message->type = _type; \
    _message->data._data_type.message = g_string_new(_string); // if _string is NULL then g_string_new will just return NULL

// initialises a hardware error valve message
void hardware_error_valve(Message *message, int valve_no, const char *string) {
    PRINTABLE_MSG(message, string, hardware_valve, HARD_ERROR_VALVE)

    message->data.hardware_valve.valve_no = valve_no;
}

// initialises a hardware error other message
void hardware_error_other(Message *message, const char *string) {
    PRINTABLE_MSG(message, string, hardware_other, HARD_ERROR_OTHER)
}

// initialises a software error message
void software_error(Message *message, const char *string) {
    PRINTABLE_MSG(message, string, software, SOFT_ERROR)
}

// initialises a keep alive message
void keep_alive(Message *message) {
    if (NULL == message)
        return;

    message->type = KEEP_ALIVE;
}

// shorthand to bail if a pointer is NULL
#define NULL_CHECK(_ptr, _root, _ret_val) \
    if (NULL == _ptr) { \
        cJSON_Delete(_root); \
        return _ret_val; \
    }

// shorthand for decoding similar messages
#define PRINTABLE_MSG_DECODE(_data_type, _type) \
    /* message type */ \
    cJSON *_data_type##_t = cJSON_CreateString(_type); \
    NULL_CHECK(_data_type##_t, root, -1) \
    cJSON_AddItemToObject(root, "type", _data_type##_t); \
    /* message */ \
    cJSON *cjson_message_##_data_type = cJSON_CreateString(message->data._data_type.message->str); \
    NULL_CHECK(cjson_message_##_data_type, root, -1) \
    cJSON_AddItemToObject(data, "message", cjson_message_##_data_type); 

// encode a message structure into a format to be transmitted
// returns the length of the encoded_message string
ssize_t encode_message(const Message *message, char **encoded_message) {
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
        // hardware error valve
        case HARD_ERROR_VALVE: ;  // ; is because C won't define a variable as the first statement after a goto
            PRINTABLE_MSG_DECODE(hardware_valve, "HARD_ERROR_VALVE")

            // valve_no
            cJSON *valve_no = cJSON_CreateNumber((double) message->data.hardware_valve.valve_no);
            NULL_CHECK(valve_no, root, -1)
            cJSON_AddItemToObject(data, "valve_no", valve_no);
            break;

        case HARD_ERROR_OTHER: ;
            PRINTABLE_MSG_DECODE(hardware_other, "HARD_ERROR_OTHER")
            break;

        case SOFT_ERROR: ;
            PRINTABLE_MSG_DECODE(software, "SOFT_ERROR")
            break;

        case KEEP_ALIVE: ;
            // message type
            cJSON *keep_alive_type = cJSON_CreateString("KEEP_ALIVE");
            NULL_CHECK(keep_alive_type, root, -1)
            cJSON_AddItemToObject(root, "type", keep_alive_type);
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

// shorthand for decoding similar message types
#define PRINTABLE_MSG_ENCODE(_type) \
    if (0 == strncmp(_type, type->valuestring, strlen(_type))) { \
        /* it was a software error packet */ \
        /* get description string */ \
        cJSON *description = cJSON_GetObjectItem(data, "message"); \
        NULL_CHECK(description, root, false) \
        EXPECT_TYPE(description, String)

// decode a string into a message structure
// returns success
bool decode_message(const char* encoded_message, Message *message) {
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

    PRINTABLE_MSG_ENCODE("SOFT_ERR")
        /* initialise message */
        /* GLIB makes a copy so we don't need to worry when calling cJSON_Delete */
        software_error(message, description->valuestring);
    } else PRINTABLE_MSG_ENCODE("HARD_ERROR_OTHER")
        // it was a hardware error other packet
        // initialise message
        // GLIB copies the message so we don't need to worry about cJSON_Delete
        hardware_error_other(message, description->valuestring);
    } else PRINTABLE_MSG_ENCODE("HARD_ERROR_VALVE")
        // it was a hardware error valve packet

        // valve_no
        cJSON *valve_no = cJSON_GetObjectItem(data, "valve_no");
        NULL_CHECK(valve_no, root, false)
        EXPECT_TYPE(valve_no, Number)

        // initialize message
        hardware_error_valve(message, valve_no->valueint, description->valuestring);
    } else if (0 == strncmp("KEEP_ALIVE", type->valuestring, 11)) {
        // it was a KEEP_ALIVE packet
        keep_alive(message);
    } else {
        // we don't know what kind of packet that is
        cJSON_Delete(root);
        return false;
    }
    
    cJSON_Delete(root);
    return true;
}

// frees dynamically allocated memory *within* a message (aka this will not free the message structure itself)
void free_message(Message *msg) {
    if (!msg)
        return;

    // free the description strings if they are present
    if (SOFT_ERROR == msg->type) {
        g_string_free(msg->data.software.message, true);
        msg->data.software.message = NULL;
    } else if (HARD_ERROR_OTHER == msg->type) {
        g_string_free(msg->data.hardware_other.message, true);
        msg->data.hardware_other.message = NULL;
    } else if (HARD_ERROR_VALVE == msg->type) {
        g_string_free(msg->data.hardware_valve.message, true);
        msg->data.hardware_valve.message = NULL;
    }

    // mark the message as free'ed
    msg->type = INVALID;
}
