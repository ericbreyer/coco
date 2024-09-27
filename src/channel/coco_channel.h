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
#include <assert.h>
#include <stdbool.h>

/**
 * @brief The status of a channel transaction.
 *
 */
enum channel_status { kOkay, kFull, kEmpty, kReadOnly, kClosed, kUnbuffTings };

#define __concat(X, Y) X##Y
#define concat(X, Y) __concat(X, Y)

/**
 * @brief "generic" type definitions
 *
 */
#define extract(T) concat(extract_, T)
#define send(T) concat(send_, T)
#define channel(T) concat(channel_, T)
#define sized_channel(T, S) concat(concat(channel_, T), concat(_, S))

/**                                                                        \
 * @brief the structure of a channel, the core of it is a circular buffer  \
 * that acts as the fifo                                                   \
 *                                                                         \
 */

struct channel_base {
    enum { kBuffered, kUnbuffered } type : 1;
    union {
        struct {
            int bufSize;
            int insertPtr;
            int count;
        } bufData;
        struct {
            int reader_waiting : 15;
            int writer_waiting : 15;
            int sync_done : 1;
        } ubufData;
    };
    int closed : 1;
    int read_ready : 1;
    int write_ready : 1;
};

bool write_ready(struct channel_base * c) {
    return c->write_ready;
}
bool read_ready(struct channel_base * c) {
    return c->read_ready;
}

/**
 * @brief close a channel
 *
 */
void close(struct channel_base * c) {
    c->closed = 1;
}

/**
 * @brief check if a channel has been closed
 *
 */
bool closed(struct channel_base * c){
    return c->closed;
}

#define INCLUDE_SIZED_CHANNEL(T, S)                                            \
    struct sized_channel(T, S) {                                               \
        struct channel(T);                                                     \
        T __buf[S > 0 ? S : 1];                                                \
    }

/**                                                                        \
 * @brief initialize an allocated channel pointer                          \
 *                                                                         \
 */
void init_channel(struct channel_base *c, int S) {
    if (S > 0) {
        (c)->bufData.bufSize = S;
        (c)->bufData.insertPtr = 0;
        (c)->bufData.count = 0;
        (c)->type = kBuffered;
    } else {
        (c)->ubufData.reader_waiting = 0;
        (c)->ubufData.writer_waiting = 0;
        (c)->ubufData.sync_done = 0;
        (c)->type = kUnbuffered;
    }
    (c)->closed = 0;
    (c)->read_ready = 0;
    (c)->write_ready = 0;
}

// /**
//  * @brief include the set of channel manipulation functions for a specific
//  * "generic" channel type
//  *
//  * @param T the type the channels contain
//  *
//  */
#define INCLUDE_CHANNEL(T)                                                     \
                                                                               \
    struct channel(T) {                                                        \
        struct channel_base;                                                   \
        T buf[0];                                                              \
    };                                                                         \
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
    static enum channel_status extract(T)(struct channel(T) * c, T * out) {           \
        switch (c->type) {                                                     \
        case kBuffered:                                                        \
            if (c->bufData.count == 0 && closed(c)) {                          \
                return kClosed;                                                \
            }                                                                  \
            while (c->bufData.count == 0) {                                    \
                coco_yield();                                                  \
            }                                                                  \
            *out = c->buf[(c->bufData.insertPtr - (c->bufData.count--) +       \
                           c->bufData.bufSize) %                               \
                          c->bufData.bufSize];                                 \
            return kOkay;                                                      \
            break;                                                             \
        case kUnbuffered:                                                      \
            if (closed(c)) {                                                   \
                *out = 0;                                                      \
                return kClosed;                                                \
            }                                                                  \
            ++c->ubufData.reader_waiting;                                      \
            do {                                                               \
                coco_yield();                                                  \
            } while (!c->ubufData.writer_waiting);                             \
            --c->ubufData.reader_waiting;                                      \
            --c->ubufData.writer_waiting;                                      \
            c->ubufData.sync_done = 1;                                         \
            *out = c->buf[0];                                                  \
            return kOkay;                                                      \
            break;                                                             \
        }                                                                      \
        assert(0);                                                             \
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
    static enum channel_status send(T)(struct channel(T) * c, T data) {               \
        switch (c->type) {                                                     \
        case kBuffered:                                                        \
            if (closed(c))                                                     \
                return kClosed;                                                \
            while (c->bufData.count == c->bufData.bufSize) {                   \
                coco_yield();                                                  \
            }                                                                  \
            c->buf[(c->bufData.insertPtr++) % c->bufData.bufSize] = data;      \
            ++c->bufData.count;                                                \
            break;                                                             \
        case kUnbuffered:                                                      \
            if (closed(c))                                                     \
                return kClosed;                                                \
            ++c->ubufData.writer_waiting;                                      \
            while (!c->ubufData.reader_waiting) {                              \
                coco_yield();                                                  \
            }                                                                  \
            c->buf[0] = data;                                                  \
            while (!c->ubufData.sync_done) {                                   \
                coco_yield();                                                  \
            }                                                                  \
            c->ubufData.sync_done = 0;                                         \
            break;                                                             \
        }                                                                      \
        return kOkay;                                                          \
    }

/**                                                                        \
 * @brief queue a member into the fifo if non-full and non-closed          \
 *                                                                         \
 * @param[in] c a pointer to the channel                                   \
 * @param[out] data the data to send                                       \
 *                                                                         \
 * @return the state of the transaction as an enum channel_status          \
 *                                                                         \
 */
enum channel_status status(struct channel_base *c) {
    switch (c->type) {
    case kBuffered:
        if (closed(c) && c->bufData.count > 0)
            return kReadOnly;
        if (closed(c))
            return kClosed;
        if (c->bufData.count == c->bufData.bufSize) {
            return kFull;
        }
        if (c->bufData.count == 0) {
            return kEmpty;
        }
        return kOkay;

        break;
    case kUnbuffered:
        if (closed(c))
            return kClosed;
        if (c->ubufData.reader_waiting) {
            return kEmpty;
        }
        if (c->ubufData.writer_waiting) {
            return kFull;
        }
        return kUnbuffTings;

        break;
    }
    assert(0);
}

void chan_select(int num_channels, struct channel_base *cs[num_channels]) {
    for (int i = 0; i < num_channels; ++i) {
        cs[i]->read_ready = 0;
        cs[i]->write_ready = 0;
        enum channel_status s = status(cs[i]);
        if (s == kOkay || s == kEmpty) {
            cs[i]->write_ready = 1;
        }
        if (s == kClosed || s == kReadOnly || s == kOkay || s == kFull) {
            cs[i]->read_ready = 1;
        }
    }
}
