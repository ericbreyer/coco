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

/**
 * @brief The status of a channel transaction.
 *
 */
enum channel_status { kOkay, kFull, kEmpty, kReadOnly, kClosed, kUnbuffTings };

// /**
//  * @brief "generic" type definitions
//  *
//  */
#define extract(T) extract_##T
#define send(T) send_##T
#define channel(T) channel_##T

#define __concat(X, Y) X##Y
#define _concat(X, Y) __concat(X, Y)
#define sized_channel(T, S) _concat(channel_##T, _##S)

/**
 * @brief close a channel
 *
 */
#define close(name) ((gen_channel *)name)->closed = 1

/**
 * @brief check if a channel has been closed
 *
 */
#define closed(name) ((gen_channel *)name)->closed

/**                                                                        \
 * @brief the structure of a channel, the core of it is a circular buffer  \
 * that acts as the fifo                                                   \
 *                                                                         \
 */

typedef struct s_gen_channel {
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
} gen_channel;

#define write_ready(c) c->write_ready
#define read_ready(c) c->read_ready

#define INCLUDE_SIZED_CHANNEL(T, S)                                            \
                                                                               \
    typedef struct _concat(s_, sized_channel(T, S)) {                          \
        channel(T);                                                            \
        T __buf[S > 0 ? S : 1];                                                \
    }                                                                          \
    sized_channel(T, S);

/**                                                                        \
 * @brief initialize an allocated channel pointer                          \
 *                                                                         \
 */
void init_channel(gen_channel * _c, int S) {
    if (S > 0) {
        (_c)->bufData.bufSize = S;
        (_c)->bufData.insertPtr = 0;
        (_c)->bufData.count = 0;
        (_c)->type = kBuffered;
    } else {
        (_c)->ubufData.reader_waiting = 0;
        (_c)->ubufData.writer_waiting = 0;
        (_c)->ubufData.sync_done = 0;
        (_c)->type = kUnbuffered;
    }
    (_c)->closed = 0;
    (_c)->read_ready = 0;
    (_c)->write_ready = 0;
}

// /**
//  * @brief include the set of channel manipulation functions for a specific
//  * "generic" channel type
//  *
//  * @param T the type the channels contain
//  * @param max the max size of the channel FIFO for this type
//  *
//  */
#define INCLUDE_CHANNEL(T)                                                     \
                                                                               \
    typedef struct _concat(s_, channel(T)) {                                   \
        gen_channel;                                                           \
        T buf[0];                                                              \
    }                                                                          \
    channel(T);                                                                \
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
    enum channel_status send(T)(channel(T) * c, T data) {                      \
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
enum channel_status status(gen_channel *c) {
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

void chan_select(gen_channel *cs[], int num_channels) {
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
