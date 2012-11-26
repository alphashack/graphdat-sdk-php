//
//  timers.h
//  graphdat-sdk-php
//
//  Created by Sugendran Ganess on 22/11/12.
//  Copyright (c) 2012 Sugendran Ganess. All rights reserved.
//

#include <stddef.h>
#include <sys/time.h>
#include "msgpack.h"

#ifndef graphdat_sdk_php_timers_h
#define graphdat_sdk_php_timers_h

// holds the items for the timers (contexts in the message)
struct graphdat_timer {
    // we move back to this index when this timer is ended
    int parentIndex;
    // used to store when the timer was started
    struct timeval lastTimerStart;
	// the name the user has given to this timer e.g. 'bar'
    char *name;
    // stores the full path e.g. '/foo/bar'
    char *fullPath;
    // time since the request started this was first called
    double firsttimestampoffset;
    // total time spent in this timer
    double responsetime;
    // how many times this timer was called
    int callcount;
    // cpu ticks spent.. zero for now
    double cputime;
};

struct graphdat_timer_list {
	// the actual array of timers
    struct graphdat_timer *array;
    // the current index we are at
    // this is so that the user can do things like
    // /foo
    // /foo/bar
    // /foo/magic
    // /foo/bar
    // where we move up and down the timer list incrementing
    // call count where appropriate
    int currentIndex;
    // size of array used up
    int used;
    // all available space in array
    int capacity;
};

void initTimerList(int initialSize, struct graphdat_timer_list *timerList);
void emptyTimerList(struct graphdat_timer_list *timerList);
void freeTimerList(struct graphdat_timer_list *timerList);
void beginTimer(struct graphdat_timer_list* timerList, char *name, struct timeval requestStart);
void endTimer(struct graphdat_timer_list* timerList, char *name);
double totalResponseTime(struct graphdat_timer_list* timerList);
void outputTimersToMsgPack(msgpack_packer* pk, struct graphdat_timer_list *timerList);

#define timeValToMs(a) (((double)a.tv_sec * 1000.0f) + ((double)a.tv_usec / 1000.0f))

#endif
