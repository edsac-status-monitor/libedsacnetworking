/*
 * Copyright 2017
 * GPL3 Licensed
 * timer.c
 * Functions relating to timers
 */

// includes
#define _POSIX_C_SOURCE 199309L
#include "edsac_timer.h"
#include <signal.h>
#include <string.h>

// functions

bool create_timer(timer_handler_t handler, timer_t *timer_id, time_t seconds) {
    struct sigevent sig_event;
    memset(&sig_event, 0, sizeof(sig_event));

    // notify a therad
    sig_event.sigev_notify = SIGEV_THREAD;
    sig_event.sigev_notify_function = (void (*) (union sigval)) handler;

    // create timer
    if (0 != timer_create(CLOCK_MONOTONIC, &sig_event, timer_id)) {
        return false;
    }

    struct itimerspec t_spec;
    t_spec.it_interval.tv_nsec = 0;
    t_spec.it_interval.tv_sec = seconds;
    t_spec.it_value.tv_nsec = 0;
    t_spec.it_value.tv_sec = seconds;

    if (0 != timer_settime(*timer_id, 0, &t_spec, NULL)) {
        return false;
    }

    return true;
}

bool stop_timer(timer_t timer_id) {
    struct itimerspec t_spec;
    memset(&t_spec, 0, sizeof(t_spec));

    return 0 != timer_settime(timer_id, 0, &t_spec, NULL);
}
