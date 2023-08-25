/**
 * @file coco_config.h
 * @author Eric Breyer (ericbreyer.com)
 * @brief Configuration defines for the scheduler
 * @version 0.2
 * @date 2023-05-27
 *
 * @copyright Copyright (c) 2023
 *
 */

#pragma once

#define MAX_TASKS (1 << 4)
#define USR_CTX_SIZE (1 << 9) // Max size of user data context segment

#define CLOCKS_TO_MS (1.0 / CLOCKS_PER_SEC)

// #define defineSP() register void * sp asm("sp")
#define defineSP() register void * sp = __builtin_frame_address(0)
/**
 * @brief functions and include to get the stack pointer for stack saving
 *
 */
#if !defined(defineSP) &&                                                         \
    (defined(WIN32) || defined(__WIN32) || defined(__WIN32__))
#include <malloc.h>
#define defineSP() void * sp = _alloca(0)
#elif !defined(defineSP)
#include <alloca.h>
#define defineSP() void * sp = alloca(0)
#endif
