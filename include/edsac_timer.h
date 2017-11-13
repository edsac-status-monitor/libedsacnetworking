/*
 * Copyright 2017
 * GPL3 Licensed
 * timer.h
 * Header for timer.c
 */

#ifndef TIMER_H 
#define TIMER_H

// link properly with C++
#ifdef _cplusplus
extern "C" {
#endif // _cplusplus

// includes
#include <stdbool.h>
#include <time.h>
#include <signal.h>

typedef void (*timer_handler_t)(union sigval *sig_event);

// declarations
bool create_timer(timer_handler_t handler, timer_t *timer_id, time_t seconds);
bool stop_timer(timer_t timer_id);

#ifdef _cplusplus
}
#endif // _cplusplus
#endif // TIMER_H
