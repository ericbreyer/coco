#pragma once
typedef void (*signalHandler)(void);

enum sig { SIGINT, SIGSTP, SIGCONT, NUM_SIGNALS };
#define SIG_MASK(sig) (1 << sig)

void default_sigint(void);
void default_sigstp(void);
void default_sigcont(void);

void kill(int tid, int signal);
void _doSignal(void);
#define doSignal() yieldable_call(_doSignal());
int sigaction(int sig, signalHandler handler);