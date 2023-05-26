#include <string.h>

#pragma once
enum C_STATUS { kOkay, kFull, kEmpty, kClosed };

#define channel(T) channel_##T
#define extract(T) extract_##T
#define send(T) send_##T

#define MAKE_CHANNEL(name, T)                                                  \
    channel(T) name;                                                           \
    INIT_CHANNEL(name, T)

#define INIT_CHANNEL(name)                                                     \
    name.bufSize = sizeof(name.buf) / sizeof(name.buf[0]);                     \
    name.insertPtr = 0;                                                        \
    name.count = 0;                                                            \
    name.closed = 0;

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
    enum C_STATUS extract(T)(channel(T) * c, T * out) {                        \
        if (c->count == 0) {                                                   \
            return kEmpty;                                                     \
        }                                                                      \
        memcpy(                                                                \
            out,                                                               \
            &c->buf[(c->insertPtr - (c->count--) + c->bufSize) % c->bufSize],  \
            sizeof(T));                                                        \
        return kOkay;                                                          \
    }                                                                          \
                                                                               \
    enum C_STATUS send(T)(channel(T) * c, T data) {                            \
        if (closed(c))                                                 \
            return kClosed;                                                    \
        if (c->count == c->bufSize) {                                          \
            return kFull;                                                      \
        }                                                                      \
        memcpy(&c->buf[(c->insertPtr++) % c->bufSize], &data, sizeof(T));      \
        ++c->count;                                                            \
        return kOkay;                                                          \
    }