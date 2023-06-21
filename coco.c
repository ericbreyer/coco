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
    // printf("run %d\n", i);
    // ctx = getContext(0);
    int ret;
    // saveStack();

    if ((ret = setjmp(tasks[i].ctx.caller)) == 0) {
        ctx = getContext(i);
        longjmp(ctx->resumePoint, 0 + 1);
    }
    // printf("done %d\n", i);
    // ctx = getContext(0);
    // restoreStack();
    // printf("done %d\n", ret);
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

        char *sp = getSP();
        ctx->frameStart = sp;
        tasks[i].func();
        // if a task falls off (returns normally), just gracefully exit for it
        coco_exit();
    }
    return ret;
}

void stopRunningTask() { tasks[currentTid].status = kStopped; }

void runTasks() {
    int i;
    for (i = 1; i <= MAX_TASKS; ++i) {
        // printf("on %d\n", i);
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

enum task_status getStatus(int tid) { return tasks[tid].status; }
struct context *getContext(int tid) {
    return (struct context *)&tasks[tid].ctx;
}

int coco_waitpid(int tid, int *exitStatus, int options) {
    for (;;) {
        if (getStatus(tid) == kDone) {
            tasks[tid].status = kDead;
            if (exitStatus) {
                *exitStatus = getContext(tid)->exitStatus;
            }
            return tid;
        }
        if (options & WNOHANG) {
            break;
        }
        yield();
    }
    return 0;
}
