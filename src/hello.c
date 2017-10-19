/*
 * Copyright 2017
 * MIT Licenced
 * hello.c
 * This is just a temporary hello world program to test that the build system is working. Also tries using cJSON.
 */

#include "hello.h"
#include <stdio.h>
#include <assert.h>
#include "contrib/cJSON.h"

// pretend library function
bool say_hello(void) {
    // test that cJSON is actually linked
    char json_string[] = "{\"hello\": \"world\"}";
    
    // do json stuff
    cJSON *root = cJSON_Parse(json_string);
    assert(NULL != root);
    cJSON *hello = cJSON_GetObjectItemCaseSensitive(root, "hello");
    assert(NULL != hello);
    
    if (cJSON_IsString(hello)) {
        char *world = hello->valuestring;
        assert(NULL != world);
        printf("hello %s!\n", world);
        cJSON_Delete(root);
        return true;
    }
    
    cJSON_Delete(root);
    return false;
}

