/**
 * file: coco.h
 *
 * Author: Eric Breyer (ericbreyer.com)
 *
 * @brief: Declaration for the core functionality of the COCO tiny
 * scheduler/runtime
 *
 * Version: 0.4
 *
 * Date: 2024-9-26
 *
 * Copyright: Copyright (c) 2023
 *
 */

#ifdef _WIN32
#pragma runtime_checks("", off)
#endif

#pragma once

#include <assert.h> ///< exit
#include <setjmp.h> ///< setjmp and longjmp
#include <stddef.h> ///< get ptrdiff_t for stack saving
#include <stdio.h>  ///< fprintf
#include <string.h> ///< memcpy
#include <time.h>   ///< for yield waits

#include "coco_config.h"

#define setjmp(x) sigsetjmp(x,1)
#define longjmp siglongjmp
#define jmp_buf sigjmp_buf

/**
 * Type: coroutine
 * A coroutine (thread/process) is a function that can be paused and resumed
 * at a later time.
 * 
 * @param[in]: void* the arguments to the coroutine
 * 
 * @return: never returns
 */
typedef void (*coroutine)(void *);
#if defined(__GNUC__)
    #define AS_COROUTINE(from_ptr) ({ \
        _Pragma("GCC diagnostic push") \
        _Pragma("GCC diagnostic ignored \"-Wcast-function-type\"") \
        coroutine result = (coroutine)(from_ptr); \
        _Pragma("GCC diagnostic pop") \
        result; \
    })
#else
    #define AS_COROUTINE(from_ptr) ((coroutine)(from_ptr))
#endif
/**
 * @brief: Start the tiny runtime
 *
 * @param[in]: kernal the first task to run
 *
 * note: coco will exit when the kernal task exits
 */
void coco_start(coroutine kernal, void *args);

/**
 * @brief: Adds a task to the scheduler
 * ingroup: functions
 * @param[in]: func the function that the task will run
 * @param[in]: args arguments to said function
 * return: the tid of the added task or 0 if task can't be added
 */
int add_task(coroutine func, void *args);

/**
 * @brief: stop the currently running task
 *
 */
void stopRunningTask();

#define COCO_WNOOPT (0)
#define COCO_WNOHANG (1 << 0)
/**
 * @brief: checks if a task has exited, reaping it and getting its exit status
 * if so
 *
 * @param[in]: tid the id of the task to wait on
 * @param[in]: opts waitpid option bit vector
 * @param[out]: exitStatus a pointer to where the exit status can be stored
 *
 * return: the tid of the reaped task (0 zero if no task was reaped)
 */
int coco_waitpid(int tid, int *exitStatus, int options);
#define coco_wait(tid) coco_waitpid(tid, NULL, WNOOPT)
#define coco_join(tid) coco_wait(tid)

/**
 * @brief: Yeild to the OS.
 *
 */
void coco_yield();
/**
 * @brief: Continuously yield to the OS for a time period
 *
 * @param[in]: ms said time period in milliseconds
 */
void yieldForMs(unsigned int ms);
/**
 * @brief: Continuously yield to the OS for a time period
 *
 * @param[in]: s said time period in seconds
 */
void yieldForS(unsigned int s);
;
/**
 * @brief: Exit the process with an exit status
 *
 * @param[in]: status the exit status
 */
void coco_exit(unsigned int stat);

/**
 * @brief Automatically reap this task when it exits
 *
 */
void coco_detach();

/**
 * @brief add a deferred procedure call to the scheduler, this call will run
 * with higher priority than other tasks but lower than an interrupt
 *
 * @param[in] func the function to run
 * @param[in] args the arguments to said function
 * @return int the tid of the added task or 0 if task can't be added
 */
int add_dpc(coroutine func, void *args);

/**
 * @brief a while loop that yields to the OS every iteration
 * 
 */
#define coco_while(cond) for (; cond; coco_yield())

int coco_fork();

typedef void (*signalHandler)(void);

enum sig {
    COCO_SIGINT,
    COCO_SIGSTP,
    COCO_SIGCONT,
    NUM_SIGNALS
};

int coco_sigaction(enum sig sig, signalHandler handler);

void coco_kill(int tid, enum sig signal);