/**
 * @file coco.h
 * @author Eric Breyer (ericbreyer.com)
 * @brief Declaration for the core functionality of the COCO tiny
 * scheduler/runtime
 * @version 0.2
 * @date 2023-05-27
 *
 * @copyright Copyright (c) 2023
 *
 */

/// @defgroup core
/// @{
#pragma once

#include <setjmp.h>
#include <time.h>
// #include <sys/time.h>

#include "channel.h"
#include "signals.h"
#include "vmac.h"

#define MAX_TASKS 10
#define USR_CTX_SIZE 256 // Max size of user data context segment

/**
 * @brief Possible states a (non-running) task can be in.
 */
enum task_status {
    kDead,     ///< Not running, memory can be repurposed
    kDone,     ///< Not running, not yet reaped
    kYielding, ///< Running normally
    kStopped,  ///< Running, execution paused
    kNew,      ///< Created but not yet running
};

/**
 * @brief A coroutine (thread/process) should only enter and exit through
 * longjmps.
 */
typedef void (*coroutine)(void);

/**
 * @brief All the context a coroutine needs to resume in the right place with
 * it's data.
 */
struct context {
    jmp_buf caller; ///< the jump point to yield control to
    jmp_buf resumeStack[20]; ///< the resume points of this task
    int resumePoint; ///< the index of the next resume point to pop
    u_int64_t sigBits; ///< a bit vector of signals
    signalHandler handlers[NUM_SIGNALS]; ///< handler callbacks for said signals
    // Yielding Data
    clock_t waitStart;
    // Exit Status
    int exitStatus;
    // User data
    void *args;
    char user[USR_CTX_SIZE];
};

/**
 * @brief Global for the current context. (Dont have access to multiple cores,
 * so only one task runs at a time).
 */
extern struct context *ctx;

/**
 * @brief Adds a task to the scheduler
 * @ingroup functions
 * @param[in] func the function that the task will run
 * @param[in] args arguments to said function
 * @return the tid of the added task
 */
int add_task(coroutine func, void *args);
void stopRunningTask();
/**
 * @brief Run all scheduled tasks
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

#define WNOOPT (0)
/**
 * @brief checks if a task has exited, reaping it and getting its exit status if
 * so
 *
 * @param[in] tid the id of the task to wait on
 * @param[in] opts waitpid option bit vector
 * @param[out] exitStatus a pointer to where the exit status can be stored
 *
 * @return the tid of the reaped task (0 zero if no task was reaped)
 */
int reappid(int tid, int *exitStatus);

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
 * @param[in, opt] status the exit status (optional - default 0)
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

/*
 * For called functions to be able to yield, they need to be called with setjmp
 * and return via longjmp (because return values on the stack get messed up when
 * yielding). Using the fact that setjmp/longjmp can pass integers, and some
 * macro trickery, we can make it seem like yieldable functions can return ints.
 * any other return values should happen through output parameters
 */

/**
 * @brief return from a function that was called with yieldable_call. A function
 * must yieldable_return if and only if it was called by yieldable_call.
 *
 * @param[in,opt] val the return uint (optional - default 0)
 *
 */
#define yieldable_return(...) VMAC(_yieldable_return, __VA_ARGS__)
#define _yieldable_return0() _yieldable_return(0)
#define _yieldable_return1(x) _yieldable_return(x)
#define _yieldable_return(x)                                                   \
    longjmp(ctx->resumeStack[--ctx->resumePoint], x + 1)

extern int temp;
/**
 * @brief Call a function that has the ability to yield.
 *
 * @param[in,opt] func the function to call
 *
 * @return the return value of the function (a uint)
 *
 */
#define yieldable_call(call)                                                   \
    ((temp = setjmp(ctx->resumeStack[ctx->resumePoint++])) == 0)               \
        ? (call, ctx->resumePoint--, 0)                                        \
        : temp - 1

/**
 * @brief Define a local context struct for a function to store local variables
 * in
 *
 */
#define DECLARE_LOCAL_CONTEXT(func, members) struct func##_ctx members;

/**
 * @brief Get the local user part of the context.
 *
 * @param[in] func the name of the current function.
 */
#define LOCAL(func) (*(struct func##_ctx *)(&ctx->user))

/**
 * @brief The system part of the context (this is standard for all tasks)
 *
 */
#define CORE (*ctx)

/**
 * @brief Start the tiny runtime
 *
 * @param[in] kernal the first task
 */
#define COCO(kernal)                                                           \
    for (int kernalid = add_task((coroutine)kernal, NULL);                     \
         getStatus(kernalid) != kDone; runTasks()) {                           \
    }

/// @}