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
#include <stdio.h>
#include <string.h>
#include <time.h>
// #include <sys/time.h>

#include "channel.h"
#include "signals.h"
#include "vmac.h"

#define MAX_TASKS 3
#define USR_CTX_SIZE 4000 // Max size of user data context segment

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
    jmp_buf caller;      ///< the jump point to yield control to
    jmp_buf resumePoint; ///< the index of the next resume point to pop
    u_int64_t sigBits;   ///< a bit vector of signals
    signalHandler handlers[NUM_SIGNALS]; ///< handler callbacks for said signals
    // Yielding Data
    clock_t waitStart;
    // Exit Status
    int exitStatus;
    // User data
    void *args;
    char savedFrame[USR_CTX_SIZE];
    void *frameStart;
    size_t frameSize;
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
#define WNOHANG (1 << 0)
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
int coco_waitpid(int tid, int *exitStatus, int options);

#ifndef getSP
#include <alloca.h>
#define getSP() alloca(0)
#endif

#define saveStack()                                                            \
    do {                                                                       \
        void *sp = getSP();                                                    \
        uintptr_t stackSize = (uintptr_t)ctx->frameStart - (uintptr_t)sp;      \
        if (stackSize > USR_CTX_SIZE) {                                        \
            fprintf(stderr, "Stack too big to store sp:%p fp:%p size:%lu\n",   \
                    sp, ctx->frameStart, stackSize);                           \
        }                                                                      \
        memcpy(ctx->savedFrame, sp, stackSize);                                \
        ctx->frameSize = stackSize;                                            \
    } while (0)

#define restoreStack()                                                         \
    do {                                                                       \
        void *sp = getSP();                                                    \
        uintptr_t stackSize = ctx->frameSize;                                  \
        memcpy(sp, ctx->savedFrame, stackSize);                                \
    } while (0)

/**
 * @brief Yeild to the OS.
 *
 */
#define yield()                                                                \
    do {                                                                       \
        saveStack();                                                           \
        if (setjmp(ctx->resumePoint) == 0) {                                   \
            longjmp(ctx->caller, kYielding);                                   \
        }                                                                      \
        restoreStack();                                                        \
        doSignal();                                                            \
    } while (0)

/**
 * @brief Yeild to the OS with a status.
 *
 * @param[in] stat an enum task_status
 */
#define yieldStatus(stat)                                                      \
    do {                                                                       \
        saveStack();                                                           \
        if (setjmp(ctx->resumePoint) == 0) {                                   \
            longjmp(ctx->caller, stat);                                        \
        }                                                                      \
        restoreStack();                                                        \
        doSignal();                                                            \
    } while (0)

/**
 * @brief Continuously yield to the OS for a time period
 *
 * @param[in] ms said time period in milliseconds
 */
#define yieldForMs(ms)                                                         \
    do {                                                                       \
        saveStack();                                                           \
        ctx->waitStart = clock();                                              \
        if (setjmp(ctx->resumePoint) == 0) {                                   \
            longjmp(ctx->caller, kYielding);                                   \
        }                                                                      \
        restoreStack();                                                        \
        doSignal();                                                            \
        if ((clock() - ctx->waitStart) * 1000 / CLOCKS_PER_SEC <               \
            ((long unsigned int)ms)) {                                         \
            saveStack();                                                       \
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
#define coco_exit(...) VMAC(_exit, __VA_ARGS__)
#define _exit0() _exit(0)
#define _exit1(i) _exit(i)
#define _exit(i)                                                               \
    do {                                                                       \
        setjmp(ctx->resumePoint);                                              \
        ctx->exitStatus = i;                                                   \
        longjmp(ctx->caller, kDone);                                           \
    } while (0)

/**
 * @brief Start the tiny runtime
 *
 * @param[in] kernal the first task
 */
#define COCO(kernal)                                                           \
    int exit; \
    for (int kernalid = add_task((coroutine)kernal, NULL);                     \
         !coco_waitpid(kernalid, &exit, WNOHANG); runTasks()) {                           \
    }

/// @}