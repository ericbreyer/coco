#include "channel.h"
#include "coco.h"
#include "vmac.h"
#include "waitgroup.h"
#include <stdio.h>

INCLUDE_CHANNEL(int, 10);

void sigstp_handler(void) {
    printf("Stopped\n");
    yieldStatus(kStopped);
    yieldable_return();
}

void sigcont_handler(void) {
    printf("Continued\n");
    yieldable_return();
}

DECLARE_LOCAL_CONTEXT(nats, {
    int c;
    channel(int) * ch;
    struct waitGroup *wg;
    uintptr_t delay;
})

void nats() {
    sigaction(SIGSTP, sigstp_handler);
    sigaction(SIGCONT, sigcont_handler);
    LOCAL(nats).ch = ((void **)CORE.args)[0];
    LOCAL(nats).delay = ((uintptr_t *)CORE.args)[1];
    LOCAL(nats).wg = ((void **)CORE.args)[2];
    yield();
    for (LOCAL(nats).c = 0; LOCAL(nats).c < 10; ++LOCAL(nats).c) {
        send(int)(LOCAL(nats).ch, LOCAL(nats).c);
        yieldForMs(LOCAL(nats).delay);
    }
    wg_done(LOCAL(nats).wg);
    close(LOCAL(nats).ch);
    exit(0);
}

DECLARE_LOCAL_CONTEXT(kernal, {
    channel(int) c;
    channel(int) c2;
    struct waitGroup wg;
    int done;
})

#define sleep(i) yieldable_call(_sleep(i))
void _sleep(int i) {
    if (i == 0)
        yieldable_return();
    yieldForS(1);
    i = sleep(--i);
    yieldable_return();
}

void kernal() {
    INIT_CHANNEL(LOCAL(kernal).c);
    INIT_CHANNEL(LOCAL(kernal).c2);
    wg_new(&LOCAL(kernal).wg);
    wg_add(&LOCAL(kernal).wg, 2);
    printf("%d\n", sleep(2));
    add_task((coroutine)nats,
             (void *[]){&LOCAL(kernal).c, (void *)(uintptr_t)250,
                        &LOCAL(kernal).wg});
    add_task((coroutine)nats,
             (void *[]){&LOCAL(kernal).c2, (void *)(uintptr_t)500,
                        &LOCAL(kernal).wg});
    for (;;) {
        yield();
        int val;
        if (extract(int)(&LOCAL(kernal).c, &val) == kOkay) {
            printf("1: %d\n", val);
            if (val == 5) {
                kill(2, SIGSTP);
            }
        }
        if (extract(int)(&LOCAL(kernal).c2, &val) == kOkay) {
            printf("2: %d\n", val);
            if (val == 9) {
                kill(2, SIGCONT);
            }
        }
        if (wg_check(&LOCAL(kernal).wg)) {
            break;
        }
    }
    exit(0);
}

int main() { COCO(kernal) }