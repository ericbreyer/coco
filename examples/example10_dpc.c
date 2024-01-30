/**
 * @file example1.c
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
INCLUDE_SIZED_CHANNEL(int, 10);

void sleep(uintptr_t time) {
    if(time == 0) {
        return;
    }
    printf("sleep\n");
    yieldForS(1);
    sleep(time - 1);    
}

void sigstp_handler(void) {
    printf("Stopped\n");
    add_dpc((coroutine)sleep, 5);
}

void sigcont_handler(void) {
    printf("Continued\n");
}

struct natsArg {
    struct sized_channel(int, 10) c;
    struct waitGroup *wg;
    long unsigned int delay;
};

void nats(struct natsArg * args) {
    coco_sigaction(COCO_SIGSTP, sigstp_handler);
    coco_sigaction(COCO_SIGCONT, sigcont_handler);
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
    init_channel(&arg2.c, 10);
    init_wg(&wg);
    arg1.delay = 250;
    arg2.delay = 500;
    arg1.wg = &wg;
    arg2.wg = &wg;
    int t1 = add_task((coroutine)nats, &arg1);
    int t2 = add_task((coroutine)nats, &arg2);
    printf("Spawn TID's (%d,%d)\n", t1, t2);
    wg_add(&wg, 2);
    coco_while(!wg_check(&wg)) {
        // if there is a value in a channel, print it
        int val;
        struct channel(int) *csel[2] = {&arg1.c, &arg2.c};
        chan_select(2, (struct channel_base **)csel);
        for (int i = 0; i < 2; ++i) {
            if (read_ready(csel[i]) && !closed(csel[i])) {
                extract(int)(csel[i], &val);
                printf("%d: %d\n", i, val);
                if (i == 0 && val == 5) {
                    coco_kill(t1, COCO_SIGSTP);
                }
                
            }
        }
        if (coco_waitpid(t2, NULL, COCO_WNOHANG)) {
            coco_kill(t1, COCO_SIGCONT);
        }
    }
    coco_waitpid(t1, NULL, COCO_WNOHANG);
    t1 = add_task((coroutine)sleep, 1);
    printf("Sleeping tid=%d\n", t1);
    coco_waitpid(t1, NULL, COCO_WNOOPT);
    printf("Done\n");
    coco_exit(0);
}

int main() { coco_start(kernal, NULL); }