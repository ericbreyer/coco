#include "signals.h"
#include "coco.h"

void kill(int tid, int signal) { getContext(tid)->sigBits |= SIG_MASK(signal); }

void _doSignal(void) {
    int stopped = ctx->sigBits & SIG_MASK(SIGSTP);
    for (int sig = 0; sig < NUM_SIGNALS; ++sig) {
        if (ctx->sigBits & SIG_MASK(sig)) {
            ctx->sigBits &= ~SIG_MASK(sig);
            ctx->handlers[sig]();
        }
    }
    if(stopped) {
        yieldStatus(kStopped);
    }
    yieldable_return();
}

int sigaction(int sig, signalHandler handler) {
    if (sig < 0 || sig >= NUM_SIGNALS) {
        return -1;
    }
    ctx->handlers[sig] = handler;
    return 0;
}

void default_sigint(void) { exit(1); }
void default_sigstp(void) { return; }

void default_sigcont(void) { return; }