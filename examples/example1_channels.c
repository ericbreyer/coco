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

// Declare use of an integer channel with buffer size 10
INCLUDE_CHANNEL(int);
INCLUDE_SIZED_CHANNEL(int, 10)

// Declare form of args that the nats routine will take
struct natsArg {
    sized_channel(int, 10) c;
    long unsigned int delay;
};

void nats() {
    // a coroutine has access to it's context
    struct natsArg *args = ctx->args;

    // count and send every args->delay Ms
    for (int c = 0; c < 10; ++c) {
        send(int)(&args->c, c);
        yieldForMs(args->delay);
    }
    // cleanup and exit with status 0 (good)
    close(&args->c);
    coco_exit(0);
}

// the first task we want to spawn
void kernal() {
    // initialize the channels and argument structs (in static memory since they
    // will be passed across stack contexts)
    static struct natsArg arg1, arg2;
    init_channel(&arg1.c, 10);
    init_channel(&arg2.c, 10);
    arg1.delay = 300;
    arg2.delay = 500;

    // dynamically add our tasks with different arguments
    int t1 = add_task((coroutine)nats, &arg1);
    int t2 = add_task((coroutine)nats, &arg2);
    printf("Spawn TID's (%d,%d)\n", t1, t2);

    coco_while (!(closed(&arg1.c) && closed(&arg2.c))) {
        // if there is a value in a channel, print it
        int val;
        channel(int) *csel[2] = {&arg1.c, &arg2.c};
        chan_select((UPCAST)csel, 2);
        for (int i = 0; i < 2; ++i) {
            if (read_ready(csel[i]) && !closed(csel[i])) {
                extract(int)(csel[i], &val);
                printf("%d: %d\n", i, val);
            }
        }
    }

    // reap the tasks
    coco_waitpid(t1, NULL, COCO_WNOOPT);
    coco_waitpid(t2, NULL, COCO_WNOOPT);
    coco_exit(0);
}

// start the scheduler with the main task
int main() { coco_start(kernal); }