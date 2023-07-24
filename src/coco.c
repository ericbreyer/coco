/**
 * @file coco.c
 * @author Eric Breyer (ericbreyer.com)
 * @brief Definition for the core functionality of the COCO tiny
 * scheduler/runtime
 * @version 0.2
 * @date 2023-05-27
 *
 * @copyright Copyright (c) 2023
 *
 */

#include "coco.h"

/**
 * @brief Structure of a task from the OS's POV.
 */
struct task {
    enum task_status status;
    struct context ctx;
    coroutine func;
};

struct context *ctx;
int currentTid;

/**
 * @brief All tasks and their contexts must be kept in program memory, since the
 * stack can get corrupted when longjmp'ing back and forth, and embedded
 * applications don't like malloc.
 *
 */
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
            return i;
        }
    }
    return 0;
}

/**
 * @brief Run a single task that has already been started/
 *
 * @param[in] i the tid of the task to run
 * @return enum task_status the status of said task after it yields
 */
enum task_status runTask(int i) {
    int ret;
    if ((ret = setjmp(tasks[i].ctx.caller)) == 0) {
        ctx = getContext(i);
        longjmp(ctx->resumePoint, 0 + 1);
    }
    return ret;
}

/**
 * @brief Start a single task that has not already been started
 *
 * @param[in] i the tid of the task to start
 * @return enum task_status the status of said task after it yields
 */
enum task_status startTask(int i) {
    int ret;
    if ((ret = setjmp(tasks[i].ctx.caller)) == 0) {
        ctx = getContext(i);

        defineSP();
        ctx->frameStart = sp;
        tasks[i].func();
        // if a task returns normally, just gracefully exit for it
        // but assert that this should never happen in debug mode
        assert(0 && "All threads should call coco_exit, never return");
        coco_exit(0);
    }
    return ret;
}

void stopRunningTask() { tasks[currentTid].status = kStopped; }

/**
 * @brief run all currently running tasks once
 *
 */
void runTasks() {
    int i;
    for (i = 1; i <= MAX_TASKS; ++i) {
        currentTid = i;
        switch (tasks[i].status) {
        case kYielding:
            tasks[i].status = runTask(i);
            break;
        case kNew:
            tasks[i].status = startTask(i);
            break;
        case kStopped:
            if (tasks[i].ctx.sigBits & SIG_MASK(COCO_SIGCONT)) {
                tasks[i].status = runTask(i);
            }
            break;
        default:
            break;
        }
    }
}
/**
 * @brief Get the Status of a task
 *
 * @param[in] tid the id of the task
 * @return enum task_status representing said status
 */
enum task_status getStatus(int tid) { return tasks[tid].status; }
struct context *getContext(int tid) {
    return (struct context *)&tasks[tid].ctx;
}

int coco_waitpid(int tid, int *exitStatus, int options) {
    for (;;) {
        if (getStatus(tid) == kDone) {
            tasks[tid].status = kDead;
            if (exitStatus != NULL) {
                *exitStatus = getContext(tid)->exitStatus;
            }
            return tid;
        }
        if (options & COCO_WNOHANG) {
            break;
        }
        coco_yield();
    }
    return 0;
}

void coco_start(coroutine kernal) {
    int exit;
    for (int kernalid = add_task((coroutine)kernal, NULL);
         !coco_waitpid(kernalid, &exit, COCO_WNOHANG); runTasks()) {
    }
}

#define saveStack()                                                            \
    do {                                                                       \
        defineSP();                                                    \
        ptrdiff_t stackSize = (char *)ctx->frameStart - (char *)sp;            \
        assert((stackSize < USR_CTX_SIZE) &&                                   \
               "Stack too big to store, increase stack storage limit or fix "  \
               "program.");                                                    \
        memcpy(ctx->savedFrame, sp, stackSize);                                \
        ctx->frameSize = stackSize;                                            \
    } while (0)

//         fprintf(stderr, "saving %ld from %p to %p\n", stackSize, ctx->frameStart, sp);

#define restoreStack()                                                         \
    do {                                                                       \
        defineSP();                                                    \
        ptrdiff_t stackSize = ctx->frameSize;                                  \
        memcpy(sp, ctx->savedFrame, stackSize);                                \
    } while (0)

void coco_yield() {
    saveStack();
    if (setjmp(ctx->resumePoint) == 0) {
        longjmp(ctx->caller, kYielding);
    } else {
    }
    restoreStack();
    _doSignal();
}

void yieldStatus(enum task_status stat) {
    saveStack();
    if (setjmp(ctx->resumePoint) == 0) {
        longjmp(ctx->caller, stat);
    } else {
    }
    restoreStack();
    _doSignal();
}

void yieldForMs(unsigned int ms) {
    saveStack();
    ctx->waitStart = clock();
    if (setjmp(ctx->resumePoint) == 0) {
        longjmp(ctx->caller, kYielding);
    } else {
    }
    restoreStack();
    _doSignal();
    if ((((clock() - ctx->waitStart) * 1000) * CLOCKS_TO_MS) < ((clock_t)ms)) {
        saveStack();
        longjmp(ctx->caller, kYielding);
    }
}
inline void yieldForS(unsigned int s) { yieldForMs(s * 1000); }

void coco_exit(unsigned int stat) {
    ctx->exitStatus = stat;
    setjmp(ctx->resumePoint);
    longjmp(ctx->caller, kDone);
}
