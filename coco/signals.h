#pragma once
typedef void (*signalHandler)(void);

enum sig { COCO_SIGINT, COCO_SIGSTP, COCO_SIGCONT, NUM_SIGNALS };
#define SIG_MASK(sig) (1 << sig)

void default_sigint(void);
void default_sigstp(void);
void default_sigcont(void);

void kill(int tid, int signal); 
void doSignal(void);
int sigaction(int sig, signalHandler handler);