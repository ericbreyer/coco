#include "channel.h"
#include <stdio.h>
#include <time.h>

enum { COUNTER_BASE = __COUNTER__ };

#define LOCAL_COUNTER (__COUNTER__ - COUNTER_BASE - 1)

enum task_status {
    COROUTINE_STATUS_DONE,
    kYeilding,
};

INCLUDE_CHANNEL(int, 10);

#define MAYBE_UNUSED(x) (void)x

#define MAX_TASKS 100
#define CTX_SIZE 4096

struct context {
    int resumePoint;
    void *args;
    clock_t waitStart;
};

struct generic_context {
    struct context core;
    int unused[CTX_SIZE - sizeof(struct context)];
};

typedef enum task_status (*coroutine)(struct generic_context *ctx);

#define MAKE_COROUTINE(name, context_t, body)                                  \
    enum COROUTINE_STATUS name(context_t *ctx) {                               \
        PRE_CO                                                                 \
        body;                                                                  \
        POST_CO                                                                \
    }

#define PRE_CO                                                                 \
    switch (((struct context *)ctx)->resumePoint) {                            \
    case -1:;

#define POST_CO                                                                \
    case -2:                                                                   \
    default:                                                                   \
        ((struct context *)ctx)->resumePoint = -2;                             \
        }                                                                      \
        return COROUTINE_STATUS_DONE;

#define YIELD()                                                                \
    ((struct context *)ctx)->resumePoint = LOCAL_COUNTER + 1;                  \
    return kYeilding;                                          \
    case LOCAL_COUNTER:

#define KILL                                                                   \
    ((struct context *)ctx)->resumePoint = LOCAL_COUNTER + 1;                  \
    case LOCAL_COUNTER:                                                        \
        return COROUTINE_STATUS_DONE;

#define YIELD_WAITFORMILLIS(ms)                                                \
    ((struct context *)ctx)->resumePoint = LOCAL_COUNTER + 1;                  \
    ((struct context *)ctx)->waitStart = clock();                              \
    case LOCAL_COUNTER:                                                        \
        if ((clock() - ((struct context *)ctx)->waitStart) * 1000 /            \
                CLOCKS_PER_SEC <                                               \
            ms) {                                                              \
            return kYeilding;                                  \
        }

#define YIELD_WAITFORSECONDS(s) YIELD_WAITFORMILLIS(s * 1000)

struct task {
    coroutine func;
    int running;
    enum task_status status;
    struct generic_context ctx;
};

static struct task tasks[MAX_TASKS];

int add_task(coroutine func, void *args) {
    for (int i = 0; i < MAX_TASKS; ++i) {
        if (tasks[i].running == 0) {
            tasks[i].func = func;
            tasks[i].running = 1;
            tasks[i].status = kYeilding;
            tasks[i].ctx = (struct generic_context){
                .core = {
                    .resumePoint = -1, .args = args, .waitStart = clock()}};
            return i;
        }
    }
    return 0;
}

void remove_task(int i) { tasks[i].running = 0; }

void runTasks() {
    for (int i = 0; i < MAX_TASKS; ++i) {
        if (tasks[i].running) {
            tasks[i].status = tasks[i].func(&tasks[i].ctx);
        }
    }

    // fflush(stdout);
}

enum task_status getStatus(int tid) { return tasks[tid].status; }

#define CUSTOM_CTX(name, fields)                                               \
    struct name {                                                              \
        struct context core;                                                   \
        fields                                                                 \
    };

struct print_ctx {
    struct context core;
    struct {
        int c;
        channel(int) * ch;
    } user;
};

MAKE_COROUTINE(nats, struct print_ctx, {
    ctx->user.ch = ctx->core.args;
    for (ctx->user.c = 0; ctx->user.c < 10; ++ctx->user.c) {
        send(int)(ctx->user.ch, ctx->user.c);
        YIELD_WAITFORMILLIS(250);
    }
    close(ctx->user.ch);
    KILL;
})

#define LOCAL ctx->user

struct kernal_context {
    struct context core;
    struct {
        channel(int) c;
        channel(int) c2;
    } user;
};

MAKE_COROUTINE(kernal, struct kernal_context, {
    INIT_CHANNEL(LOCAL.c);
    INIT_CHANNEL(LOCAL.c2);
    add_task((coroutine)nats, &LOCAL.c);
    add_task((coroutine)nats, &LOCAL.c2);
    for (;;) {
        int val;
        if (extract(int)(&LOCAL.c, &val) == kOkay)
            printf("1: %d\n", val);
        if (extract(int)(&LOCAL.c2, &val) == kOkay)
            printf("2: %d\n", val);
        if(closed(&LOCAL.c) && closed(&LOCAL.c)) {
            KILL;
        }
        YIELD();
    }
})

#define SCHEDULER for (; getStatus(0) != COROUTINE_STATUS_DONE; runTasks())
#define USE_EOS                                                                \
    int main() {                                                               \
        add_task((coroutine)kernal, NULL);                                     \
        SCHEDULER;                                                             \
    }

USE_EOS;

// WAIT - yeild till task is done + reaping
// WAITPID - that behavior