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

// Declare use of an integer channel with buffer size 10
INCLUDE_CHANNEL(int);
INCLUDE_SIZED_CHANNEL(int, 0);

struct counter {
    struct sized_channel(int, 0) inc;
    struct sized_channel(int, 0) read;
    struct sized_channel(int, 0) ctx;
};

void servicer(struct counter * c) {
    int i = 0;
    int j;
    struct channel_base *csel[3] = {&c->inc, &c->read, &c->ctx};
    while(1) {
        chan_select(3, csel);
        if (read_ready(&c->inc)) {
            extract(int)(&c->inc, &j);
            ++i;
        }
        if (write_ready(&c->read)){
            send(int)(&c->read, i);
        }
        if (read_ready(&c->ctx)) {
            break;
        }
        coco_yield();
    }
    coco_exit(0);
}

void init_counter(struct counter * c) {
    init_channel(&c->inc, 0);
    init_channel(&c->read, 0);
    init_channel(&c->ctx, 0);
    add_task((coroutine)servicer, c);
}

void inc(struct counter * c) {
    send(int)(&c->inc, 1);
}
int read(struct counter * c) {
    int ret;
    extract(int)(&c->read, &ret);
    return ret;
}
void done(struct counter * c) {
    close(&c->ctx);
}

void inc100(struct counter * c) {
    for (int i = 0; i < 100; ++i) {
        coco_yield();
        inc(c);
    }
}

struct A {
    int i;
};

struct B {
    struct A;
};

// the first task we want to spawn
void kernal() {

    static struct counter c;
    init_counter(&c);
    int tids[5];
    for (int i = 0; i < 5; ++i) {
        tids[i] = add_task((coroutine)inc100, &c);
    }
    for (int i = 0; i < 5; ++i) {
        inc100(&c);
    }
    for (int i = 0; i < 5; ++i) {
        coco_waitpid(tids[i], NULL, COCO_WNOOPT);
    }
    int val = read(&c);
    done(&c);
    coco_yield();
    printf("%d\n", val);

    if(val != 1000) {
        coco_exit(1);
    }

    coco_exit(0);
    
}

// start the scheduler with the main task
int main() { coco_start(kernal, NULL); }