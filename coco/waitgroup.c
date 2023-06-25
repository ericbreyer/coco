/**
 * @file waitgroup.c
 * @author Eric Breyer (ericbreyer.com)
 * @brief Definitions for go-like wait group constructs in the COCO tiny
 * scheduler/runtime. Inspired by go waitgroups.
 * @version 0.2
 * @date 2023-05-27
 *
 * @copyright Copyright (c) 2023
 *
 */

#include "waitgroup.h"
#include "coco.h"

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