#pragma once
#include "coco.h"

struct waitGroup {
    unsigned int counter;
};

void wg_new(struct waitGroup *wg) { wg->counter = 0; }

void wg_add(struct waitGroup *wg, unsigned int numTasks) {
    wg->counter += numTasks;
}

void wg_done(struct waitGroup *wg) { --wg->counter; }

int wg_check(struct waitGroup *wg) { return wg->counter == 0; }

#define wg_wait(wg)                                                            \
    do {                                                                       \
        for (;;) {                                                             \
            if (wg->counter == 0) {                                            \
                yieldable_return(0);                                           \
            }                                                                  \
            yield();                                                           \
        }                                                                      \
    } while (0)