@mainpage
# coco
coco is a tiny, lightwight, portable, and cooperative scheduler for C. It's use and primatives are heavily based of off Go's goroutines, channels and waitgroups. Coco can be used to as a simple library for "Green" threads and coroutines in C, while signals make coco act like a small concurrent (though not parallel) operating system.

## coco features:
### core scheduler

- Task concurency
- Dynamic task creation
- Task contexts for stack data saving and restoration
- Yield points and yields for a time period
- Yeilds can be arbitrarly deap in a subroutine call tree
- Task exit status
- Task reaping to obtain exit status and check aliveness
- No dynamic memory allocations behind the scenes
- Defered procedure call for interupt and signal handling

### signals
- Inspired by UNIX-style signals
- Default signal handlers
- Install custom signal actions
- Ability to stop (freeze) and continue tasks dynamically with SIGSTP and SIGCONT

### Go-style channels and waitgroups
- channels provide a FIFO queues for inter-task communication
- waitgroups provide a mechanism for a task to wait on spawned children

## examples:
```c

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "channel.h"
#include "coco.h"

// Declare use of an integer channel with buffer size 10
INCLUDE_CHANNEL(int, 10);

// Declare form of args that the nats routine will take
struct natsArg {
    channel(int) c;
    long unsigned int delay;
};

void nats() {
    // a coroutine has access to it's context
    struct natsArg *args = getArgs();

    // count and send every args->delay Ms
    for (int c = 0; c < 10; ++c) {
        send(int)(&args->c, c);
        yieldForMs(args->delay);
    }
    // cleanup and exit with status 0 (good)
    close(&args->c);
    coco_exit(0);
}

// the first task we want to spawn
void kernal() {
    // initialize the channels and argument structs (in static memory since they
    // will be passed across stack contexts)
    static struct natsArg arg1, arg2;
    init_channel(&arg1.c);
    init_channel(&arg2.c);
    arg1.delay = 300;
    arg2.delay = 500;

    // dynamically add our tasks with different arguments
    int t1 = add_task((coroutine)nats, &arg1);
    int t2 = add_task((coroutine)nats, &arg2);
    printf("Spawn TID's (%d,%d)\n", t1, t2);

    for (;;) {
        // let the OS do other things
        coco_yield();

        // if there is a value in a channel, print it
        int val;
        if (extract(int)(&arg1.c, &val) == kOkay) {
            printf("1: %d\n", val);
        }
        if (extract(int)(&arg2.c, &val) == kOkay) {
            printf("2: %d\n", val);
        }

        // we are done checking channels when they are both closed
        if (closed(&arg1.c) && closed(&arg2.c)) {
            break;
        }
    }

    // reap the tasks
    coco_waitpid(t1, NULL, COCO_WNOOPT);
    coco_waitpid(t2, NULL, COCO_WNOOPT);
    coco_exit(0);
}

// start the scheduler with the first task
int main() { coco_start(kernal); }
```
