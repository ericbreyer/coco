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
int temp;

/**
 * @brief All tasks and their contexts must be kept in program memory, since the
 * stack can get corrupted when longjmping back and forth, and embedded
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
            tasks[i].ctx.resumePoint = 0;
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
        longjmp(ctx->resumeStack[--ctx->resumePoint], 0 + 1);
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
        tasks[i].func();
    }
    return ret;
}

void stopRunningTask() {
    tasks[currentTid].status = kStopped;
}

void runTasks() {
    for (int i = 1; i <= MAX_TASKS; ++i) {
        currentTid = i;
        switch (tasks[i].status) {
        case kYielding:
            tasks[i].status = runTask(i);
            break;
        case kNew:
            tasks[i].status = startTask(i);
            break;
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

int reappid(int tid, int *exitStatus) {
    if (getStatus(tid) == kDone) {
        tasks[tid].status = kDead;
        if (exitStatus) {
            *exitStatus = getContext(tid)->exitStatus;
        }
        return tid;
    }
    return 0;
}