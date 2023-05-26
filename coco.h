#pragma once

#include <setjmp.h>
#include <time.h>
// #include <sys/time.h>

#include "channel.h"
#include "signals.h"
#include "vmac.h"

// Max tasks and size in bytes of user thread data segment.
#define MAX_TASKS 10
#define CTX_SIZE 256

// Possible states a (non-running) task can be in.
enum task_status {
    kDead,
    kDone,
    kYielding,
    kStopped,
    kNew,
};

// A coroutine (thread/process) should only enter and exit through longjmps.
typedef void (*coroutine)(void);

// All the context a coroutine needs to resume in the right place with it's
// data.
struct context {
    // Calling contexts
    jmp_buf caller;
    jmp_buf resumeStack[20];
    int resumePoint;
    // Signals
    u_int64_t sigBits;
    signalHandler handlers[NUM_SIGNALS];
    // Yielding Data
    clock_t waitStart;
    // Exit Status
    int exitStatus;
    // User data
    void *args;
    char user[CTX_SIZE];
};

// Global for the current context. (Dont have access to multiple CPUs, so only
// one task runs at a time).
struct context *ctx;

/**
 * @brief Adds a task to the scheduler
 * 
 * @param[in] func the function that the task will run
 * @param[in] args arguments to said function
 * @return the tid of the added task
 */
int add_task(coroutine func, void *args);

/**
 * @brief Runs all tasks
 * 
 */
void runTasks();

/**
 * @brief Get the running status of a task
 * 
 * @param[in] tid the id of the task
 * @return enum task_status 
 */
enum task_status getStatus(int tid);

/**
 * @brief Get the Context of a task
 * 
 * @param[in] tid the id of the task
 * @return struct context* 
 */
struct context *getContext(int tid);

/**
 * @brief sends a signal to a task
 * 
 * @param[in] tid the id of the task
 * @param[in] signal the signal to send, must be a valid, defined signal
 */
void kill(int tid, int signal);

#define WNOOPT (0)
#define WNOHANG (1 << 0)
/**
 * @brief waits for a task to exit, reaping it and getting its exit status
 * 
 * @param[in] tid the id of the task to wait on
 * @param[in] opts waitpid option bit vector
 * @param[out] exitStatus a pointer to where the exit status can be stored
 * 
 * @return the tid of the reaped task (0 zero if no task was reaped)
 */
#define waitpid(tid, opts, status) yieldable_call(_waitpid(tid, opts, status))
void _waitpid(int tid, int opts, int *exitStatus);

/**
 * @brief Yeild to the OS.
 * 
 */
#define yield()                                                                \
    do {                                                                       \
        if (setjmp(ctx->resumeStack[ctx->resumePoint++]) == 0) {               \
            longjmp(ctx->caller, kYielding);                                   \
        }                                                                      \
        doSignal();                                                            \
    } while (0)


/**
 * @brief Yeild to the OS with a status.
 * 
 * @param[in] stat an enum task_status
 */
#define yieldStatus(stat)                                                      \
    do {                                                                       \
        if (setjmp(ctx->resumeStack[ctx->resumePoint++]) == 0) {               \
            longjmp(ctx->caller, stat);                                        \
        }                                                                      \
        doSignal();                                                            \
    } while (0)

/**
 * @brief Continuously yield to the OS for a time period
 * 
 * @param[in] ms said time period in milliseconds
 */
#define yieldForMs(ms)                                                         \
    do {                                                                       \
        ctx->waitStart = clock();                                              \
        if (setjmp(ctx->resumeStack[ctx->resumePoint++]) == 0) {               \
            longjmp(ctx->caller, kYielding);                                   \
        }                                                                      \
        doSignal();                                                            \
        if ((clock() - ctx->waitStart) * 1000 / CLOCKS_PER_SEC < ms) {         \
            ctx->resumePoint++;                                                \
            longjmp(ctx->caller, kYielding);                                   \
        }                                                                      \
    } while (0)

/**
 * @brief Continuously yield to the OS for a time period
 * 
 * @param[in] s said time period in seconds
 */
#define yieldForS(s) yieldForMs(s * 1000)

/**
 * @brief Exit the process with an exit status
 * 
 * @param[in, opt] 
 * 
 */
#define exit(...) VMAC(_exit, __VA_ARGS__)
#define _exit0() _exit(0)
#define _exit1(i) _exit(i)
#define _exit(i)                                                               \
    do {                                                                       \
        setjmp(ctx->resumeStack[ctx->resumePoint]);                            \
        ctx->resumePoint++;                                                    \
        ctx->exitStatus = i;                                                   \
        longjmp(ctx->caller, kDone);                                           \
    } while (0)

// For called functions to be able to yield, they need to be called with setjmp
// and return via longjmp (because return values on the stack get messed up when
// yielding). Using the fact that setjmp/longjmp can pass integers, and some
// macro trickery, we can make it seem like yieldable functions can return ints.
// any other return values should happen through
#define yieldable_return(...) VMAC(_yieldable_return, __VA_ARGS__)
#define _yieldable_return0() _yieldable_return(0)
#define _yieldable_return1(x) _yieldable_return(x)
#define _yieldable_return(x)                                                   \
    longjmp(ctx->resumeStack[--ctx->resumePoint], x + 1)

int temp;
#define yieldable_call(call)                                                   \
    ((temp = setjmp(ctx->resumeStack[ctx->resumePoint++])) == 0)               \
        ? (call, ctx->resumePoint--, 0)                                        \
        : temp - 1


// Shortcuts to access parts of the context.
#define LOCAL(type) (*(type *)(&ctx->user))
#define CORE (*ctx)