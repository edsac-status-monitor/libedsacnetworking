/*
 * Copyright 2017
 * MIT Licence
 * hello.h
 * Just temporary to see if the build system is working
 */

#ifndef HELLO_H
#define HELLO_H

// link properly with C++
#ifdef _cplusplus
extern "C" {
#endif // _cplusplus

// includes
#include<stdbool.h>

// declarations
bool say_hello(void);

#ifdef _cplusplus
}
#endif // _cplusplus
#endif // HELLO_H