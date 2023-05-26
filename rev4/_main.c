#include "channel.h"
#include "coco.h"
#include "vmac.h"
#include <stdio.h>

INCLUDE_CHANNEL(int, 10);

typedef struct print_ctx {
    int c;
    channel(int) * ch;
    uintptr_t delay;
} nats_t;

void nats() {
    LOCAL(nats_t)->ch = ((void **)CORE->args)[0];
    LOCAL(nats_t)->delay = ((uintptr_t *)CORE->args)[1];
    yield();
    for (LOCAL(nats_t)->c = 0; LOCAL(nats_t)->c < 10; ++LOCAL(nats_t)->c) {
        send(int)(LOCAL(nats_t)->ch, LOCAL(nats_t)->c);
        YIELD_WAITFORMILLIS(LOCAL(nats_t)->delay);
    }
    CLOSE_CHANNEL(LOCAL(nats_t)->ch);
    KILL;
}

typedef struct kernal_context {
    channel(int) c;
    channel(int) c2;
    int done;
} kernal_t;

void sleep() {
    YIELD_WAITFORSECONDS(1);
}

void kernal() {
    INIT_CHANNEL(LOCAL(kernal_t)->c);
    INIT_CHANNEL(LOCAL(kernal_t)->c2);
    LOCAL(kernal_t)->done = 0;
    printf("here\n");
    printf("here\n");
    add_task((coroutine)nats,
             (void *[]){&LOCAL(kernal_t)->c, (void *)(uintptr_t)250});
    add_task((coroutine)nats,
             (void *[]){&LOCAL(kernal_t)->c2, (void *)(uintptr_t)500});
             YIELD_WAITFORSECONDS(1);
                              sleep();
    for (;;) {
        yield();
        int val;
        if (extract(int)(&LOCAL(kernal_t)->c, &val) == kOkay) {
            printf("1: %d\n", val);
            continue;
        }
        if (extract(int)(&LOCAL(kernal_t)->c2, &val) == kOkay) {
            printf("2: %d\n", val);
            continue;
        }
        if (waitpid(3, WAIT_OPT_NOHANG))
            LOCAL(kernal_t)->done++;
        if (waitpid(2, WAIT_OPT_NOHANG))
            LOCAL(kernal_t)->done++;
        if (LOCAL(kernal_t)->done == 2)
            break;
    }
    KILL;
}
#define COCO(kernal)                                                           \
    for (int kernal_tid = add_task((coroutine)kernal, NULL);                   \
         getStatus(kernal_tid) != kDone; runTasks()) {                         \
    }

int main() { COCO(kernal) }

// WAITGROUPS