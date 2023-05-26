#pragma once
#include "coco.h"

struct waitGroup {
    unsigned int counter;
};

void wg_new(struct waitGroup * wg) {
    wg->counter = 0;
}

void wg_add(struct waitGroup * wg, unsigned int numTasks) {
    wg->counter += numTasks;
}

void wg_done(struct waitGroup * wg) {
    --wg->counter;
}

int wg_check(struct waitGroup * wg) {
    return wg->counter == 0;
}

#define wg_wait(wg) coco_call(_wg_wait(wg))
void _wg_wait(struct waitGroup * wg) {
    if(wg->counter == 0) {
        yieldable_return(0);
    }
    yield();
}