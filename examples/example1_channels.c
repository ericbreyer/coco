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

#include "channel.h"
#include "coco.h"

INCLUDE_CHANNEL(int,
                10); // Declare use of an integer channel with buffer size 10

struct natsArg {
    channel(int) * c;
    long unsigned int delay;
};

void nats() {
    struct natsArg *args = ctx->args;
    for (int c = 0; c < 10; ++c) {
        send(int)(args->c, c);
        yieldForMs(args->delay);
    }
    close(args->c);
    coco_exit(0);
}

void kernal() {
    struct natsArg *arg1 = malloc(sizeof *arg1);
    struct natsArg *arg2 = malloc(sizeof *arg2);
    arg1->c = malloc(sizeof *arg1->c);
    arg2->c = malloc(sizeof *arg2->c);
    init_channel(arg1->c);
    init_channel(arg2->c);
    arg1->delay = 300;
    arg2->delay = 500;
    int t1 = add_task((coroutine)nats, arg1);
    int t2 = add_task((coroutine)nats, arg2);
    printf("Spawn TID's (%d,%d)\n", t1, t2);
    for (;;) {
        coco_yield();
        int val;
        if (extract(int)(arg1->c, &val) == kOkay) {
            printf("1: %d\n", val);
        }
        if (extract(int)(arg2->c, &val) == kOkay) {
            printf("2: %d\n", val);
        }
        if (closed(arg1->c) && closed(arg2->c)) {
            break;
        }
    }
    coco_exit(0);
}

int main() { coco_start(kernal); }