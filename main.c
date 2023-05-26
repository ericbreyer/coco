#include "channel.h"
#include "coco.h"
#include "vmac.h"
#include "waitgroup.h"
#include <stdio.h>

INCLUDE_CHANNEL(int, 10);

typedef struct print_ctx {
    int c;
    channel(int) * ch;
    struct waitGroup *wg;
    uintptr_t delay;
} nats_t;

void sigstp_handler(void) {
    printf("Stopped\n");
    yieldStatus(kStopped);
    yieldable_return();
}

void sigcont_handler(void) {
    printf("Continued\n");
    yieldable_return();
}

void nats() {
    sigaction(SIGSTP, sigstp_handler);
    sigaction(SIGCONT, sigcont_handler);
    LOCAL(nats_t).ch = ((void **)CORE.args)[0];
    LOCAL(nats_t).delay = ((uintptr_t *)CORE.args)[1];
    LOCAL(nats_t).wg = ((void **)CORE.args)[2];
    yield();
    for (LOCAL(nats_t).c = 0; LOCAL(nats_t).c < 10; ++LOCAL(nats_t).c) {
        send(int)(LOCAL(nats_t).ch, LOCAL(nats_t).c);
        yieldForMs(LOCAL(nats_t).delay);
    }
    wg_done(LOCAL(nats_t).wg);
    close(LOCAL(nats_t).ch);
    exit(0);
}

typedef struct kernal_context {
    channel(int) c;
    channel(int) c2;
    struct waitGroup wg;
    int done;
} kernal_t;

#define sleep(i) yieldable_call(_sleep(i))
void _sleep(int i) {
    if (i == 0)
        yieldable_return();
    yieldForS(1);
    i = sleep(--i);
    yieldable_return();
}

void kernal() {
    INIT_CHANNEL(LOCAL(kernal_t).c);
    INIT_CHANNEL(LOCAL(kernal_t).c2);
    wg_new(&LOCAL(kernal_t).wg);
    wg_add(&LOCAL(kernal_t).wg, 2);
    printf("%d\n",     sleep(2));
    add_task((coroutine)nats,
             (void *[]){&LOCAL(kernal_t).c, (void *)(uintptr_t)250,
                        &LOCAL(kernal_t).wg});
    add_task((coroutine)nats,
             (void *[]){&LOCAL(kernal_t).c2, (void *)(uintptr_t)500,
                        &LOCAL(kernal_t).wg});
    for (;;) {
        yield();
        int val;
        if (extract(int)(&LOCAL(kernal_t).c, &val) == kOkay) {
            printf("1: %d\n", val);
            if (val == 5) {
                kill(2, SIGSTP);
            }
        }
        if (extract(int)(&LOCAL(kernal_t).c2, &val) == kOkay) {
            printf("2: %d\n", val);
            if (val == 9) {
                kill(2, SIGCONT);
            }
        }
        if (wg_check(&LOCAL(kernal_t).wg)) {
            break;
        }
    }
    exit(0);
}
#define COCO(kernal)                                                           \
    for (int kernal_tid = add_task((coroutine)kernal, NULL);                   \
         getStatus(kernal_tid) != kDone; runTasks()) {                         \
    }

int main() { COCO(kernal) }

// WAITGROUPS
// SIGNALS check
// EXIT STATUS