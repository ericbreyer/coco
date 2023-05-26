#include "coco.h"

struct task {
    enum task_status status;
    struct context ctx;
};

static struct task tasks[MAX_TASKS + 1];

int add_task(coroutine func, void *args) {
    for (int i = 1; i < MAX_TASKS; ++i) {
        if (tasks[i].status == kDead) {
            tasks[i].status = kNew;
            tasks[i].ctx = (struct context){.args = args, .waitStart = clock(), .func = func};
            return i;
        }
    }
    return 0;
}

void remove_task(int i) { tasks[i].status = kDead; }

enum task_status runTask(int i) {
    int ret;
    if ((ret = setjmp(tasks[i].ctx.caller)) == 0) {

        ctx = getContext(i);
        longjmp(tasks[i].ctx.resumePoint, i);
    }
    return ret;
}

enum task_status startTask(int i) {
    int ret;
    if ((ret = setjmp(tasks[i].ctx.caller)) == 0) {
        ctx = getContext(i);
        ctx->func();
    }
    return ret;
}

void runTasks() {
    for (int i = 0; i < MAX_TASKS; ++i) {
        if (tasks[i].status == kYielding) {
            tasks[i].status = runTask(i);
        }
        else if (tasks[i].status == kNew) {
            tasks[i].status = startTask(i);
        }
    }
}

enum task_status getStatus(int tid) { return tasks[tid].status; }
struct context *getContext(int tid) {
    return (struct context *)&tasks[tid].ctx;
}

int waitpid(int tid, int opts) {
    for (;;) {
        if (getStatus(tid) == kDone) {
            tasks[tid].status = kDead;
            return tid;
        }
        if (opts & WAIT_OPT_NOHANG)
            return 0;
        yield();
    }
}

void doSignal() {
    return;
    if ((ctx)->signal) {
        (ctx)->handlers[(ctx)->signal]();
    }
}