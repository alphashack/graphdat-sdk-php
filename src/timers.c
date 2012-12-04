//
//  timers.c
//  graphdat-sdk-php
//
//  Created by Sugendran Ganess on 22/11/12.
//  Copyright (c) 2012 Sugendran Ganess. All rights reserved.
//

#include "timers.h"
#include "php.h"
#include <string.h>

void initTimerList(int initialSize, struct graphdat_timer_list *timerList)
{
    uint arraySize = initialSize * sizeof(struct graphdat_timer);
    timerList->array = (struct graphdat_timer *) malloc(arraySize);
    timerList->used = 0;
    timerList->currentIndex = -1;
    timerList->capacity = initialSize;
    if(timerList->array == NULL)
    {
        zend_error(E_WARNING, "Could not allocate %d bytes for Graphdat timers", arraySize);
    }
}

void emptyTimerList(struct graphdat_timer_list *timerList)
{
    if(timerList->array == NULL)
    {
        return;
    }
    int i;
    for(i=0; i<timerList->used; i++)
    {
        struct graphdat_timer *timer = (struct graphdat_timer*) &timerList->array[i];
        if(timer->name)
        {
            free(timer->name);
            timer->name = NULL;
        }
        if(timer->fullPath)
        {
            free(timer->fullPath);
            timer->fullPath = NULL;
        }
    }
    timerList->currentIndex = -1;
    timerList->used = 0;
}

void freeTimerList(struct graphdat_timer_list *timerList)
{
    emptyTimerList(timerList);
    if(timerList->array)
    {
        free(timerList->array);
        timerList->array = NULL;
    }
    timerList->used = timerList->capacity = 0;
}

int indexOfTimer(struct graphdat_timer_list *timerList, char *fullPath)
{
    int result = -1;
    int i = MAX(0, timerList->currentIndex);
    for(;result == -1 && i<timerList->used; i++)
    {
        struct graphdat_timer *timer = (struct graphdat_timer*) &timerList->array[i];
        if(strcmp(timer->fullPath, fullPath) == 0)
        {
            result = i;
        }
    }
    return result;
}

// start a timer
// store the offset from request start to this timer in firsttimestampoffset
// store the cumulative time spent in this timer in responsetime (i.e. sum multiple hits to this timer)
// callcount is the number of times this timer was called
void beginTimer(struct graphdat_timer_list* timerList, char *name, struct timeval requestStart)
{
    if(timerList->array == NULL)
    {
        return;
    }
    struct graphdat_timer *timer;
    char *currentPath = (timerList->currentIndex < 1) ? "" : timerList->array[timerList->currentIndex].fullPath;
    size_t fullPathLen = strlen(currentPath) + strlen(name) + 3;
    char *fullPath = emalloc(fullPathLen);
    zend_sprintf(fullPath, "%s/%s", currentPath, name);
    // now find that fullPath in the list
    int indx = (timerList->currentIndex == -1) ? -1 : indexOfTimer(timerList, fullPath);
    if(indx == -1)
    {
        struct timeval tnow;
        if(timerList->used == timerList->capacity)
        {
            timerList->capacity *= 2;
            timerList->array = (struct graphdat_timer *)realloc(timerList->array, timerList->capacity * sizeof(struct graphdat_timer));
        }
        timer = (struct graphdat_timer*) &timerList->array[timerList->used];
        indx = timerList->used;
        timerList->used++;

        gettimeofday(&tnow, NULL);

        timer->name = strdup(name);
        timer->fullPath = strdup(fullPath);
        timer->firsttimestampoffset = (timeValToMs(tnow) - timeValToMs(requestStart));
        timer->responsetime = 0;
        timer->callcount = 0;
        timer->cputime = 0;
        timer->parentIndex = timerList->currentIndex;
    }
    else
    {
        timer = (struct graphdat_timer*) &timerList->array[indx];
    }
    efree(fullPath);
    timer->callcount++;
    timerList->currentIndex = indx;
    gettimeofday(&(timer->lastTimerStart), NULL);
}

void endTimer(struct graphdat_timer_list* timerList, char *name)
{
    if(timerList->array == NULL)
    {
        return;
    }
    if(timerList->currentIndex < 0 || timerList->currentIndex >= timerList->used)
    {
        zend_error(E_ERROR, "Could not end timer named '%s' since the current index is out of bounds.", name);
        return;
    }
    if(strcmp(timerList->array[timerList->currentIndex].name, name) != 0)
    {
        zend_error(E_ERROR, "Could not end timer named '%s' since it's not the last timer to begin.", name);
        return;
    }
    struct timeval timeNow;
    struct graphdat_timer *timer = (struct graphdat_timer*) &timerList->array[timerList->currentIndex];
    gettimeofday(&timeNow, NULL);
    timer->responsetime += (timeValToMs(timeNow) - timeValToMs(timer->lastTimerStart));
    timerList->currentIndex = timer->parentIndex;
}

double totalResponseTime(struct graphdat_timer_list* timerList)
{
    if(timerList->used < 1)
    {
        return 0.0;
    }
    return timerList->array[0].responsetime;
}

void outputTimersToMsgPack(msgpack_packer* pk, struct graphdat_timer_list *timerList)
{
    int i;
    if(timerList->used == 0)
    {
        return;
    }
    msgpack_pack_array(pk, timerList->used);
    // context has 5 items (firsttimestampoffset, responsetime, callcount, cputime, name)
    for(i=0; i<timerList->used; i++)
    {
        struct graphdat_timer *timer = (struct graphdat_timer*) &timerList->array[i];
        msgpack_pack_map(pk, 5);
        // firsttimestampoffset
        msgpack_pack_raw(pk, sizeof("firsttimestampoffset") - 1);
        msgpack_pack_raw_body(pk, "firsttimestampoffset", sizeof("firsttimestampoffset") - 1);
        msgpack_pack_double(pk, timer->firsttimestampoffset);
        // responsetime
        msgpack_pack_raw(pk, sizeof("responsetime") - 1);
        msgpack_pack_raw_body(pk, "responsetime", sizeof("responsetime") - 1);
        msgpack_pack_double(pk, timer->responsetime);
        // callcount
        msgpack_pack_raw(pk, sizeof("callcount") - 1);
        msgpack_pack_raw_body(pk, "callcount", sizeof("callcount") - 1);
        msgpack_pack_int(pk, timer->callcount);
        // cputime
        msgpack_pack_raw(pk, sizeof("cputime") - 1);
        msgpack_pack_raw_body(pk, "cputime", sizeof("cputime") - 1);
        msgpack_pack_double(pk, timer->cputime);
        // name
        msgpack_pack_raw(pk, sizeof("name") - 1);
        msgpack_pack_raw_body(pk, "name", sizeof("name") - 1);
        msgpack_pack_raw(pk, strlen(timer->fullPath));
        msgpack_pack_raw_body(pk, timer->fullPath, strlen(timer->fullPath));
    }
}