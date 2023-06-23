#include "channel.h"
#include "coco.h"
#include "vmac.h"
#include "waitgroup.h"
#include <stdbool.h>
#include <stdio.h>
// #include <stdlib.h>
#include <string.h>

INCLUDE_CHANNEL(int,
                10); // Declare use of an integer channel with buffer size 10

void sigstp_handler(void) { printf("Stopped\n"); }

void sigcont_handler(void) { printf("Continued\n"); }

struct natsArg {
    channel(int) * c;
    struct waitGroup *wg;
    long unsigned int delay;
};

void nats() {
    struct natsArg *args = ctx->args;
    sigaction(COCO_SIGSTP, sigstp_handler);
    sigaction(COCO_SIGCONT, sigcont_handler);
    for (int c = 0; c < 10; ++c) {
        send(int)(args->c, c);
        yieldForMs(args->delay);
    }
    wg_done(args->wg);
    close(args->c);
    coco_exit(0);
}

void sleep() {
    printf("sleep\n");
    yieldForS(1);
}

void kernal() {
    struct natsArg *arg1 = malloc(sizeof *arg1);
    struct natsArg *arg2 = malloc(sizeof *arg2);
    arg1->c = constuctChannel(int)();
    arg2->c = constuctChannel(int)();
    struct waitGroup *wg = wg_new();
    arg1->delay = 250;
    arg2->delay = 500;
    arg1->wg = wg;
    arg2->wg = wg;
    int t1 = add_task((coroutine)nats, arg1);
    int t2 = add_task((coroutine)nats, arg2);
    wg_add(wg, 2);
    for (;;) {
        yield();
        int val;
        if (extract(int)(arg1->c, &val) == kOkay) {
            printf("1: %d\n", val);
            if (val == 5) {
                kill(t1, COCO_SIGSTP);
            }
        }
        if (extract(int)(arg2->c, &val) == kOkay) {
            printf("2: %d\n", val);
        }
        if (coco_waitpid(t2, NULL, WNOHANG)) {
            kill(t1, COCO_SIGCONT);
        }

        if (wg_check(wg)) {
            break;
        }
    }
    t1 = add_task((coroutine)sleep, NULL);
    coco_waitpid(t1, NULL, WNOOPT);
    sleep();
    coco_exit(0);
}

int main() { COCO(kernal); }