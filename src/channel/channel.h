/**
 * @file channel.h
 * @author Eric Breyer (ericbreyer.com)
 * @brief Definitions to create FIFO queues for inter-task communication.
 * Inspired heavily by go channels.
 * @version 0.2
 * @date 2023-05-27
 *
 * @copyright Copyright (c) 2023
 *
 */

#pragma once

/**
 * @brief The status of a channel transaction.
 *
 */
enum channel_status { kOkay, kFull, kEmpty, kClosed };

/**
 * @brief "generic" type definitions
 *
 */
#define channel(T) channel_##T
#define extract(T) extract_##T
#define send(T) send_##T

/**
 * @brief initialize an allocated channel pointer
 *
 */
#define init_channel(name)                                                     \
    do {                                                                       \
        (name)->bufSize = sizeof((name)->buf) / sizeof((name)->buf[0]);        \
        (name)->insertPtr = 0;                                                 \
        (name)->count = 0;                                                     \
        (name)->closed = 0;                                                    \
    } while (0)

/**
 * @brief close a channel
 *
 */
#define close(name) (name)->closed = 1

/**
 * @brief check if a channel has been closed
 *
 */
#define closed(name) (name)->closed

/**
 * @brief include the set of channel manipulation functions for a specific
 * "generic" channel type
 *
 * @param T the type the channels contain
 * @param max the max size of the channel FIFO for this type
 *
 */
#define INCLUDE_CHANNEL(T, max)                                                \
    /**                                                                        \
     * @brief the structure of a channel, the core of it is a circular buffer  \
     * that acts as the fifo                                                   \
     *                                                                         \
     */                                                                        \
    typedef struct {                                                           \
        T buf[max];                                                            \
        int bufSize;                                                           \
        int insertPtr;                                                         \
        int count;                                                             \
        int closed;                                                            \
    } channel(T);                                                              \
                                                                               \
    /**                                                                        \
     * @brief extract a member from the fifo if non-empty                      \
     *                                                                         \
     * @param[in] c a pointer to the channel                                   \
     * @param[out] out a reference to a space to store the extracted value     \
     *                                                                         \
     * @return the state of the transaction as an enum channel_status          \
     *                                                                         \
     */                                                                        \
    enum channel_status extract(T)(channel(T) * c, T * out) {                  \
        if (c->count == 0) {                                                   \
            return closed(c) ? kClosed : kEmpty;                               \
        }                                                                      \
        *out =                                                                 \
            c->buf[(c->insertPtr - (c->count--) + c->bufSize) % c->bufSize];   \
        return kOkay;                                                          \
    }                                                                          \
                                                                               \
    /**                                                                        \
     * @brief queue a member into the fifo if non-full and non-closed          \
     *                                                                         \
     * @param[in] c a pointer to the channel                                   \
     * @param[out] data the data to send                                       \
     *                                                                         \
     * @return the state of the transaction as an enum channel_status          \
     *                                                                         \
     */                                                                        \
    enum channel_status send(T)(channel(T) * c, T data) {                      \
        if (closed(c))                                                         \
            return kClosed;                                                    \
        if (c->count == c->bufSize) {                                          \
            return kFull;                                                      \
        }                                                                      \
        c->buf[(c->insertPtr++) % c->bufSize] = data;                          \
        ++c->count;                                                            \
        return kOkay;                                                          \
    }
