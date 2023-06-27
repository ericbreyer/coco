/**
 * @file waitgroup.h
 * @author Eric Breyer (ericbreyer.com)
 * @brief Declarations for go-like wait group constructs in the COCO tiny
 * scheduler/runtime. Inspired by go waitgroups.
 * @version 0.2
 * @date 2023-05-27
 *
 * @copyright Copyright (c) 2023
 *
 */

#pragma once

/**
 * @brief a wait group is really just an atomic counter, since this is not
 * multithreaded or preemptive, everything is already atomic.
 *
 */
struct waitGroup {
    unsigned int counter;
};

/**
 * @brief initialize an allocated waitgroup pointer
 *
 * @param[in] wg the pointer to initialize
 */
void init_wg(struct waitGroup *wg);

/**
 * @brief add a number of tasks to wait on
 *
 * @param[in] wg the wait group
 * @param[in] numTasks the number of tasks
 */
void wg_add(struct waitGroup *wg, unsigned int numTasks);

/**
 * @brief subtract one from the wait group counter
 *
 * @param[in] wg the wait group
 */
void wg_done(struct waitGroup *wg);

/**
 * @brief check if the counter is equal to zero (all tasks are done)
 *
 * @param[in] wg the wait group
 * @return 1 counter is 0, 0 otherwise
 */
int wg_check(struct waitGroup *wg);

/**
 * @brief yield until a waitgroup's count is 0
 *
 * @param[in] wg
 */
void wg_wait(struct waitGroup *wg);