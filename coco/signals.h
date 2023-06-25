/**
 * @file signals.h
 * @author Eric Breyer (ericbreyer.com)
 * @brief Declaration for the signal sending/handling in the COCO tiny
 * scheduler/runtime. Inspired by POSIX signals.
 * @version 0.2
 * @date 2023-05-27
 *
 * @copyright Copyright (c) 2023
 *
 */

#pragma once

/**
 * @brief the required type of a signal handler
 * 
 */
typedef void (*signalHandler)(void);

enum sig { COCO_SIGINT, COCO_SIGSTP, COCO_SIGCONT, NUM_SIGNALS };

/**
 * @brief utility macro for creating signal bitvectors
 * 
 */
#define SIG_MASK(sig) (1 << sig)

/**
 * @brief default handlers before a sigaction is installed for that signal
 *
 */
void default_sigint(void);
void default_sigstp(void);
void default_sigcont(void);

/**
 * @brief send a signal to a task
 *
 * @param[in] tid the id of the task to target
 * @param[in] signal the signal to send
 */
void kill(int tid, enum sig signal);

/**
 * @brief handle all currently pending signals (auto called when returning from
 * yield)
 *
 */
void _doSignal(void);

/**
 * @brief install a custom signal handler for a signal for this task
 *
 * @param[in] sig the signal to handle with a custom function
 * @param[in] handler the callback function when this signal is seen
 * @return -1 if signal is invalid, 0 on success
 *
 * @note COCO_SIGSTP will cause the task to stop regardless of the handler
 * callback, do not call yieldStatus(kStopped) here, COCO_SIGINT will not
 * automatically exit, however.
 */
int sigaction(enum sig sig, signalHandler handler);