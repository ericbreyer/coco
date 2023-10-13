/**
 * @file example_waitgroups.c
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

INCLUDE_CHANNEL(int); // Declare use of an integer channel with buffer size 10
INCLUDE_SIZED_CHANNEL(int, 10)

struct natsArg {
    sized_channel(int, 10) c;
    struct waitGroup *wg;
    long unsigned int delay;
};

void nats() {
    coco_detach();
    struct natsArg * args = ctx->args;
    for (int c = 0; c < 10; ++c) {
        send(int)(&args->c, c);
        yieldForMs(args->delay);
    }
    wg_done(args->wg);
    close(&args->c);
    coco_exit(0);
}

void kernal() {
    static struct natsArg arg1, arg2;
    static struct waitGroup wg;

    init_channel(&arg1.c, 10);
    init_channel(&arg2.c,10);
    init_wg(&wg);
    arg1.delay = 250;
    arg2.delay = 500;
    arg1.wg = &wg;
    arg2.wg = &wg;
    wg_add(&wg, 2);
    int t1 = add_task((coroutine)nats, &arg1);
    int t2 = add_task((coroutine)nats, &arg2);
    printf("Spawn TID's (%d,%d)\n", t1, t2);
    coco_while(!wg_check(&wg)) {

        // if there is a value in a channel, print it
        int val;
        channel(int) * csel[2] = {&arg1.c, &arg2.c};
        chan_select((UPCAST)csel, 2);
        for (int i = 0; i < 2; ++i) {
            if (read_ready(csel[i]) && !closed(csel[i])) {
                extract(int)(csel[i], &val);
                printf("%d: %d\n", i, val);
            }
        }
    }
    coco_exit(0);
}

int main() { coco_start(kernal); }


// read ready function