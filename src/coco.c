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
#include <stdbool.h>
#include <stdlib.h>

/**
 * Struct: context
 *
 * All the context a coroutine needs to resume in the right place with
 * it's data.
 *
 */
struct context {
    jmp_buf caller;      // The paused context of the caller
    jmp_buf resumePoint; // The paused context of the coroutine
    signalHandler
        handlers[NUM_SIGNALS]; // The signal handlers for this coroutine
    clock_t waitStart; // The time at which this coroutine started waiting
    int exitStatus;    // The exit status of this coroutine
    void *args;        // The arguments passed to this coroutine
    char savedFrame[USR_CTX_SIZE]; // The saved stack frame of this coroutine
    void *frameStart;              // The start of the context's stack frame
    ptrdiff_t frameSize;           // The size of the context's stack frame
    int detached; // Whether this coroutine is detached from it's parent
};

/**
 * Enum: task_status
 *
 * Possible states a (non-running) task can be in.
 *
 * - kUDead: dead
 * - kDead: dead, memory can be repurposed
 * - kDone: finished, not yet reaped
 * - kYielding: running normally
 * - kStopped: running, execution paused
 * - kNew: task created and queued to run
 *
 */
enum task_status {
    kUDead,
    kDead,
    kDone,
    kYielding,
    kStopped,
    kNew,
};

/**
 * @brief Structure of a task from the OS's POV.
 */
struct task {
    enum task_status status; // The status of the task
    struct context ctx;      // The context of the task
    coroutine func;          // The function to run for the task
    struct task *next;       // The next task in the list
    struct task *prev;       // The previous task in the list
};

static struct context *ctx;      // The context of the currently running task
static struct task *currentTask; // The currently running task
static bool can_yield = true;    // Whether the current task can yield

/**
 * @brief All tasks and their contexts must be kept in program memory, since the
 * stack can get corrupted when longjmp'ing back and forth, and embedded
 * applications don't like malloc.
 *
 */
static struct task tasks[MAX_TASKS];
static struct task runningTasks;
static struct task freeTasks;
static struct task dpcs;

/**
 * @brief insert a node into a circular doubly linked list
 *
 * @param[in] l the node to insert after
 * @param[in] node the node to insert
 */
void cdll_insert(struct task *l, struct task *node) {
    node->next = l->next;
    node->prev = l;
    l->next->prev = node;
    l->next = node;
}

/**
 * @brief remove a node from a circular doubly linked list
 *
 * @param[in] node the node to remove
 */
void cdll_remove(struct task *node) {
    node->prev->next = node->next;
    node->next->prev = node->prev;
}

/**
 * @brief print a circular doubly linked list
 *
 * @param[in] tasks the list to print
 */
void cdll_print(struct task *tasks) {
    printf("[%ld <- %ld -> %ld] ", tasks->prev - tasks, tasks - tasks,
           tasks->next - tasks);
    for (struct task *node = tasks->next; node != tasks; node = node->next) {
        printf("[%ld <- %ld -> %ld] ", node->prev - tasks, node - tasks,
               node->next - tasks);
    }
    printf("\n");
}

void default_sigint(void) { coco_exit(1); }
void default_sigstp(void) { return; }
void default_sigcont(void) { return; }
/**
 * @brief Initialize a task for use in the scheduler
 *
 * @param[in] t the task to initialize
 * @param[in] func the function to run for the task
 * @param[in] args the arguments to pass to the function
 */
void init_task(struct task *t, coroutine func, void *args) {
    t->status = kNew;
    t->func = func,
    t->ctx = (struct context){
        .args = args,
        .waitStart = clock(),
        .handlers = {default_sigint, default_sigstp, default_sigcont},
        .detached = false};
}

int add_task_to_queue(coroutine func, void *args, struct task *list) {
    for (struct task *node = freeTasks.next; node != &freeTasks;
         node = node->next) {
        if (node->status == kDead || node->status == kUDead) {
            init_task(node, func, args);
            cdll_remove(node);
            cdll_insert(list, node);
            return node - tasks;
        }
    }
    assert(false && "No more tasks available");
    return 0;
}

/**
 * @brief Add a task to the scheduler
 *
 * @param[in] func the function to run for the task
 * @param[in] args the arguments to pass to the function
 * @return int the tid of the task
 */
int add_task(coroutine func, void *args) {
    return add_task_to_queue(func, args, &runningTasks);
}

/**
 * @brief Add a task to the dpc queue
 *
 * @param[in] func the function to run for the task
 * @param[in] args the arguments to pass to the function
 * @return int the tid of the task
 */
int add_dpc(coroutine func, void *args) {
    int tid = add_task_to_queue(func, args, &dpcs);
    tasks[tid].ctx.detached = true;
    return tid;
}

/**
 * @brief Run a single task that has already been started/
 *
 * @param[in] i the tid of the task to run
 * @return enum task_status the status of said task after it yields
 */
enum task_status runTask(struct task *t) {
    int ret;
    if ((ret = setjmp(t->ctx.caller)) == 0) {
        ctx = &t->ctx;
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
enum task_status startTask(struct task *t) {
    int ret;
    if ((ret = setjmp(t->ctx.caller)) == 0) {
        ctx = &t->ctx;

        defineSP();
        ctx->frameStart = sp;
        t->func(ctx->args);
        // if a task returns normally, just gracefully exit for it
        // but assert that this should never happen in debug mode
        coco_exit(0);
    }
    return ret;
}

void stopRunningTask() { currentTask->status = kStopped; coco_yield(); }

void runDPCs() {
    while (1) {
        struct task *next = NULL;
        for (struct task *t = dpcs.next; t != &dpcs; t = next) {
            currentTask = t;
            next = t->next;
            switch (t->status) {
            case kYielding:
                t->status = runTask(t);
                break;
            case kNew:
                t->status = startTask(t);
                break;
            default:
                break;
            }
        }
        if (dpcs.next == &dpcs) {
            break;
        }
    } // PUT THEM ON DIFFERENT THREADS
}

/**
 * @brief run all currently running tasks once
 *
 */
void runTasks() {
    struct task *next = NULL;
    for (struct task *t = runningTasks.next; t != &runningTasks;
         t = next) {
        runDPCs();
        currentTask = t;
        next = currentTask->next;
        switch (currentTask->status) {
        case kYielding:
            currentTask->status = runTask(currentTask);
            break;
        case kNew:
            currentTask->status = startTask(currentTask);
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

/**
 * @brief: Get the Context of a task
 *
 * @param[in]: tid the id of the task
 * return: struct context*
 */
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
            cdll_insert(&freeTasks, &tasks[tid]);
            return tid;
        }
        if (options & COCO_WNOHANG) {
            break;
        }
        coco_yield();
    }
    return 0;
}

void coco_start(coroutine kernal, void *args) {
    int texit;
    freeTasks.next = &freeTasks;
    freeTasks.prev = &freeTasks;
    runningTasks.next = &runningTasks;
    runningTasks.prev = &runningTasks;
    dpcs.next = &dpcs;
    dpcs.prev = &dpcs;
    for (int i = MAX_TASKS; i >= 1; --i) {
        cdll_insert(&freeTasks, &tasks[i]);
    }

    for (int kernalid = add_task((coroutine)kernal, args);
         !coco_waitpid(kernalid, &texit, COCO_WNOHANG);) {
        runTasks();
    }
    exit(texit);
}

#define saveStack()                                                            \
    do {                                                                       \
        defineSP();                                                            \
        ptrdiff_t stackSize = (char *)ctx->frameStart - (char *)sp;            \
        assert((stackSize < USR_CTX_SIZE) &&                                   \
               "Stack too big to store, increase stack storage limit or fix "  \
               "program.");                                                    \
        memcpy(ctx->savedFrame, sp, stackSize);                                \
        ctx->frameSize = stackSize;                                            \
    } while (0)

#define restoreStack()                                                         \
    do {                                                                       \
        defineSP();                                                            \
        ptrdiff_t stackSize = ctx->frameSize;                                  \
        memcpy(sp, ctx->savedFrame, stackSize);                                \
    } while (0)

void coco_yield() {
    if (!can_yield) {
        assert(false && "Can't yield here");
    }
    saveStack();
    if (setjmp(ctx->resumePoint) == 0) {
        longjmp(ctx->caller, kYielding);
    } else {
    }
    restoreStack();
}

void yieldForMs(unsigned int ms) {
    if (!can_yield) {
        assert(false && "Can't yield here");
    }
    saveStack();
    ctx->waitStart = clock();
    if (setjmp(ctx->resumePoint) == 0) {
        longjmp(ctx->caller, kYielding);
    } else {
    }
    restoreStack();
    if (((clock() - ctx->waitStart) * CLOCKS_TO_MS) < ((clock_t)ms)) {
        saveStack();
        longjmp(ctx->caller, kYielding);
    }
}
inline void yieldForS(unsigned int s) { yieldForMs(s * 1000); }

void coco_detach() { ctx->detached = true; }

void coco_exit(unsigned int stat) {
    if (!can_yield) {
        assert(false && "Can't yield here");
    }
    ctx->exitStatus = stat;
    setjmp(ctx->resumePoint);
    cdll_remove(currentTask);
    if (ctx->detached) {
        cdll_insert(&freeTasks, currentTask);
    }
    longjmp(ctx->caller, ctx->detached ? kDead : kDone);
}

size_t next_free_task() {
    for (size_t i = 1; i <= MAX_TASKS; ++i) {
        if (tasks[i].status == kDead || tasks[i].status == kUDead) {
            return i;
        }
    }
    return 0;
}

int coco_fork() {
    volatile size_t tid = next_free_task();
    if (tid == 0) {
        return 0;
    }
    struct task *childTask = &tasks[tid];
    cdll_remove(&tasks[tid]);
    cdll_insert(&runningTasks, &tasks[tid]);
    childTask->status = kYielding;

    saveStack();
    struct context *child = &childTask->ctx;
    memcpy(child, ctx, sizeof(struct context));
    if (setjmp(child->resumePoint) != 0) {
        restoreStack();
        return 0;
    }
    return tid;
}

void coco_kill(int tid, enum sig signal) {
    struct context *ctx = getContext(tid);
    can_yield = false;
    ctx->handlers[signal]();
    can_yield = true;
    switch (signal) {
    case COCO_SIGSTP:
        tasks[tid].status = kStopped;
        break;
    case COCO_SIGCONT:
        tasks[tid].status = kYielding;
        break;
    default:
        break;
    }
}

int coco_sigaction(enum sig sig, signalHandler handler) {
    if (sig < 0 || sig >= NUM_SIGNALS) {
        return -1;
    }
    ctx->handlers[sig] = handler;
    return 0;
}