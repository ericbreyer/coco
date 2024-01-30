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

#include "coco.h"
#include "coco_channel.h"
#include "waitgroup.h"

// Declare use of an integer channel with buffer size 10
INCLUDE_CHANNEL(int);
INCLUDE_SIZED_CHANNEL(int, 0);

struct counter {
    struct sized_channel(int, 0) count;
};

struct send_args {
    struct counter *c;
    int n;
};

void send_num(struct send_args *a) {
    printf("Sending %d\n", a->n);
    coco_detach();
    send(int)(&a->c->count, a->n);
    free(a);
    coco_exit(0);
}

void counter_increment(struct counter *this) {
    struct channel(int) *c = &this->count;
    int curr;
    extract(int)(c, &curr);
    curr++;
    struct send_args *a = malloc(sizeof *a);
    a->c = this;
    a->n = curr;
    add_task((coroutine)send_num, a);
}

int counter_read(struct counter *this, int final) {
    struct channel(int) *c = &this->count;
    int val;
    extract(int)(c, &val);
    if (final) {
        close(c);
    } else {
        struct send_args *a = malloc(sizeof *a);
        a->c = this;
        a->n = val;
        add_task((coroutine)send_num, a);
    }
    return val;
}

void construct_counter(struct counter *this) {
    init_channel(&this->count, 0);
    struct send_args *a = malloc(sizeof *a);
    a->c = this;
    a->n = 0;
    add_task((coroutine)send_num, a);
}

// the first task we want to spawn
void kernal() {

    static struct counter c;
    construct_counter(&c);
    int tids[100];
    for (int i = 0; i < 100; ++i) {
        tids[i] = add_task((coroutine)counter_increment, &c);
        printf("Spawn %d\n", tids[i]);
    }
    for (int i = 0; i < 100; ++i) {
        counter_increment(&c);

    }
    for (int i = 0; i < 100; ++i) {
        coco_waitpid(tids[i], NULL, COCO_WNOOPT);
    }
    int val = counter_read(&c, true);
    printf("%d\n", val);
    coco_exit(0);
}

// start the scheduler with the main task
int main() { coco_start(kernal, NULL); }