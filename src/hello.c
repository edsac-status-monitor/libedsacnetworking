/*
 * Copyright 2017
 * MIT Licenced
 * hello.c
 * This is just a temporary hello world program to test that the build system is working
 */

#include "hello.h"
#include <stdio.h>

// pretend library function
bool say_hello(void) {
    printf("Hello world!\n");

    return true;
}

// pretend unit test
#ifdef TEST
#include <stdlib.h> // EXIT_*
int main(void) {
    if (say_hello())
        return EXIT_SUCCESS;
    else
        return EXIT_FAILURE;
}
#endif // TEST