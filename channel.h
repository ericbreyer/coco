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
 * @brief The status of the channel.
 *
 */
enum channel_status { kOkay, kFull, kEmpty, kClosed };

#define constuctChannel(T) constuctChannel_##T
#define channel(T) channel_##T
#define extract(T) extract_##T
#define send(T) send_##T

#define INIT_CHANNEL(name)                                                     \
    (name).bufSize = sizeof((name).buf) / sizeof((name).buf[0]);               \
    (name).insertPtr = 0;                                                      \
    (name).count = 0;                                                          \
    (name).closed = 0;

#define close(name) (name)->closed = 1;
#define closed(name) (name)->closed

#define INCLUDE_CHANNEL(T, max)                                                \
    typedef struct {                                                           \
        T buf[max];                                                            \
        int bufSize;                                                           \
        int insertPtr;                                                         \
        int count;                                                             \
        int closed;                                                            \
    } channel(T);                                                              \
                                                                               \
    channel(T) * constuctChannel(T)() {                                        \
        channel(T) *ret = malloc(sizeof *ret);                                 \
        INIT_CHANNEL(*ret);                                                    \
        return ret;                                                            \
    }                                                                          \
                                                                               \
    enum channel_status extract(T)(channel(T) * c, T * out) {                  \
        if (c->count == 0) {                                                   \
            return kEmpty;                                                     \
        }                                                                      \
        *out =                                                                 \
            c->buf[(c->insertPtr - (c->count--) + c->bufSize) % c->bufSize];   \
        return kOkay;                                                          \
    }                                                                          \
                                                                               \
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
