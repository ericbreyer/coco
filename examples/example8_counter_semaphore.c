/**
 * @file example_channels.c
 * @author Eric Breyer (ericbreyer.com)
 * @brief Demo of use of coco constructs
 * @version 0.2
 * @date 2023-05-27
 *
 * @copyright Copyright (c) 2023
 *
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "coco_channel.h"
#include "coco.h"
#include "waitgroup.h"
#include "semaphore.h"

// Declare use of an integer channel with buffer size 10
INCLUDE_CHANNEL(int);
INCLUDE_SIZED_CHANNEL(int, 0);

struct counter {
    struct sized_channel(int, 0) count;
};
coco_sem sem;


void counter_increment(struct counter *this) {
    struct channel(int) *c =  & this->count;
    int curr;
    extract(int)(c, &curr);
    coco_yield();
    curr++;
    send(int)(c, curr);
}

int counter_read(struct counter *this) {
    struct channel(int) *c =  & this->count;
    int val;
    extract(int)(c, &val);
    send(int)(c, val);
    return val;
}

void construct_counter(struct counter *this) {
    init_channel(&this->count, 1);
    send(int)(&this->count, 0);
}

struct inc_arg {
    struct counter c;
    struct waitGroup wg;
};

void inc_task(struct inc_arg *arg) {
    counter_increment(&arg->c);
    wg_done(&arg->wg);
        coco_sem_post(&sem);
    coco_exit(0);
}

// the first task we want to spawn
void kernal() {
    static struct inc_arg arg;
    construct_counter(&arg.c);
    init_wg(&arg.wg);
    wg_add(&arg.wg, 100);
    coco_sem_init(&sem, 1);
    for (int i = 0; i < 100; ++i) {
        coco_sem_wait(&sem);
        add_task((coroutine)inc_task, &arg);
    }
    int tids[100];
    for (int i = 0; i < 100; ++i) {
        tids[i] = add_task((coroutine)counter_increment, &arg.c);
    }
    wg_wait(&arg.wg);
    for (int i = 0; i < 100; ++i) {
        coco_waitpid(tids[i], NULL, COCO_WNOOPT);
    }
    int val = counter_read(&arg.c);
    printf("%d\n", val);
    coco_exit(0);
}

// start the scheduler with the main task
int main() { coco_start(kernal, NULL); }