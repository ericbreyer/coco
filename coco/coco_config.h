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

#define MAX_TASKS (1 << 7)
#define USR_CTX_SIZE (1 << 11) // Max size of user data context segment