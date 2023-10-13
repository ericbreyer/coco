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

#ifdef _WIN32
#pragma runtime_checks( "", off )
#endif

#pragma once

#include <assert.h> ///< exit
#include <setjmp.h> ///< setjmp and longjmp
#include <stddef.h> ///< get ptrdiff_t for stack saving
#include <stdio.h>  ///< fprintf
#include <string.h> ///< memcpy
#include <time.h>   ///< for yield waits


#include "coco_config.h"
#include "./signals/signals.h"

#define UPCAST void *

/**
 * @brief Possible states a (non-running) task can be in.
 */
enum task_status {
    kUDead,
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
#pragma pack(push, 1)
struct context {
    jmp_buf caller;        ///< the jump point to yield control to
    jmp_buf resumePoint;   ///< the index of the next resume point to pop
    long long int sigBits; ///< a bit vector of signals
    signalHandler handlers[NUM_SIGNALS]; ///< handler callbacks for said signals
    // Yielding Data
    clock_t waitStart;
    // Exit Status
    int exitStatus;
    // User data
    void *args;
    char savedFrame[USR_CTX_SIZE];
    void *frameStart;
    ptrdiff_t frameSize;
    int detached;
};
#pragma pack(pop)
/**
 * @brief Global for the current context. (Dont have access to multiple cores,
 * so only one task runs at a time).
 */
extern struct context *ctx;

/**
 * @brief Start the tiny runtime
 *
 * @param[in] kernal the first task to run
 *
 * @note coco will exit when the kernal task exits
 */
void coco_start(coroutine kernal);

/**
 *
 * @brief Adds a task to the scheduler
 * @ingroup functions
 * @param[in] func the function that the task will run
 * @param[in] args arguments to said function
 * @return the tid of the added task or 0 if task can't be added
 */
int add_task(coroutine func, void *args);

/**
 * @brief stop the currently running task
 *
 */
void stopRunningTask();

/**
 * @brief Get the Context of a task
 *
 * @param[in] tid the id of the task
 * @return struct context*
 */
struct context *getContext(int tid);

#define COCO_WNOOPT (0)
#define COCO_WNOHANG (1 << 0)
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
#define coco_wait(tid) coco_waitpid(tid, NULL, WNOOPT)
#define coco_join(tid) coco_wait(tid)

/**
 * @brief Yeild to the OS.
 *
 */
void coco_yield();
/**
 * @brief Yeild to the OS with a status.
 *
 * @param[in] stat an enum task_status
 */
void yieldStatus(enum task_status stat);
/**
 * @brief Continuously yield to the OS for a time period
 *
 * @param[in] ms said time period in milliseconds
 */
void yieldForMs(unsigned int ms);
/**
 * @brief Continuously yield to the OS for a time period
 *
 * @param[in] s said time period in seconds
 */
void yieldForS(unsigned int s);;
/**
 * @brief Exit the process with an exit status
 *
 * @param[in] status the exit status
 */
void coco_exit(unsigned int stat);

void coco_detach();

int add_dpc(coroutine func, void *args);

#define lambda(return_type, function_body)                                     \
    ({ return_type __fn__ function_body; __fn__; })

#define unary_to_thread(func)                                                  \
    lambda(                                                                    \
        void, (void) {                                                         \
            func(ctx->args);                                                           \
            coco_exit(0);                                                      \
        })

#define coco_while(cond) for(;cond;coco_yield())

/// @}