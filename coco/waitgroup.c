#include "waitgroup.h"

void init_wg(struct waitGroup *wg) { wg->counter = 0; }

void wg_add(struct waitGroup *wg, unsigned int numTasks) {
    wg->counter += numTasks;
}

void wg_done(struct waitGroup *wg) { --wg->counter; }

int wg_check(struct waitGroup *wg) { return wg->counter == 0; }

void wg_wait(struct waitGroup *wg) {
    for (;;) {
        if (wg->counter == 0) {
            return;
        }
        yield();
    }
}