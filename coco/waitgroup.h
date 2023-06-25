#pragma once
#include "coco.h"
#include <stdlib.h>

struct waitGroup {
    unsigned int counter;
};

struct waitGroup *wg_new();

void init_wg(struct waitGroup *wg);

void wg_add(struct waitGroup *wg, unsigned int numTasks);

void wg_done(struct waitGroup *wg);

int wg_check(struct waitGroup *wg);

void wg_wait(struct waitGroup *wg);