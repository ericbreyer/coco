#include "coco.h"

// Structure of a task.
struct task {
    enum task_status status;
    struct context ctx;
    coroutine func;
};

// All tasks and contexts must be kept in program memory.
static struct task tasks[MAX_TASKS + 1];

int add_task(coroutine func, void *args) {
    for (int i = 1; i <= MAX_TASKS; ++i) {
        if (tasks[i].status == kDead) {
            tasks[i].status = kNew;
            tasks[i].func = func,
            tasks[i].ctx = (struct context){
                .args = args,
                .waitStart = clock(),
                .handlers = {default_sigint, default_sigstp, default_sigcont}};
            tasks[i].ctx.resumePoint = 0;
            return i;
        }
    }
    return 0;
}

enum task_status runTask(int i) {
    int ret;
    if ((ret = setjmp(tasks[i].ctx.caller)) == 0) {
        ctx = getContext(i);
        yieldable_return(0);
    }
    return ret;
}

enum task_status startTask(int i) {
    int ret;
    if ((ret = setjmp(tasks[i].ctx.caller)) == 0) {
        ctx = getContext(i);
        tasks[i].func();
    }
    return ret;
}

void runTasks() {
    for (int i = 1; i <= MAX_TASKS; ++i) {
        switch (tasks[i].status) {
        case kYielding:
            tasks[i].status = runTask(i);
            break;
        case kNew:
            tasks[i].status = startTask(i);
        case kStopped:
            if (tasks[i].ctx.sigBits & SIG_MASK(SIGCONT)) {
                tasks[i].status = runTask(i);
            }
            break;
        default:
            break;
        }
    }
}

enum task_status getStatus(int tid) { return tasks[tid].status; }
struct context *getContext(int tid) {
    return (struct context *)&tasks[tid].ctx;
}

void _waitpid(int tid, int opts, int *exitStatus) {
    for (;;) {
        if (getStatus(tid) == kDone) {
            tasks[tid].status = kDead;
            if(exitStatus) {
                *exitStatus = getContext(tid)->exitStatus;
            }
            yieldable_return(tid);
        }
        if (opts & WNOHANG) {
            yieldable_return(0);
        }
        yield();
    }
}