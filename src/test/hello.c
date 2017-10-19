/*
 * Copyright 2017
 * MIT Licenced
 * test/hello.c
 * Testsuite for hello.c
 */

#include "hello.h"
#include <stdlib.h> // EXIT_*

int main(void) {
    if (say_hello())
        return EXIT_SUCCESS;
    else
        return EXIT_FAILURE;
}

