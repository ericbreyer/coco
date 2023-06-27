/**
 * @file signals.c
 * @author Eric Breyer (ericbreyer.com)
 * @brief Definitions for the signal sending/handling in the COCO tiny
 * scheduler/runtime. Inspired by POSIX signals.
 * @version 0.2
 * @date 2023-05-27
 *
 * @copyright Copyright (c) 2023
 *
 */

#include "signals.h"
#include "coco.h"

void kill(int tid, enum sig signal) { getContext(tid)->sigBits |= SIG_MASK(signal); }

void _doSignal(void) {
    int stopped = ctx->sigBits & SIG_MASK(COCO_SIGSTP);
    for (int sig = 0; sig < NUM_SIGNALS; ++sig) {
        if (ctx->sigBits & SIG_MASK(sig)) {
            ctx->sigBits &= ~SIG_MASK(sig);
            ctx->handlers[sig]();
        }
    }
    if(stopped) {
        yieldStatus(kStopped);
    }
}

int sigaction(enum sig sig, signalHandler handler) {
    if (sig < 0 || sig >= NUM_SIGNALS) {
        return -1;
    }
    ctx->handlers[sig] = handler;
    return 0;
}

void default_sigint(void) { coco_exit(1); }
void default_sigstp(void) { return; }
void default_sigcont(void) { return; }