#include "channel.h"
#include <setjmp.h>
#include <stdio.h>
#include <time.h>

enum { COUNTER_BASE = __COUNTER__ };

#define LOCAL_COUNTER (__COUNTER__ - COUNTER_BASE - 1)

enum task_status {
    COROUTINE_STATUS_DONE = 1,
    kYeilding,
};

INCLUDE_CHANNEL(int, 10);

#define MAYBE_UNUSED(x) (void)x

#define MAX_TASKS 100
#define CTX_SIZE 8096

struct context {
    jmp_buf caller;
    jmp_buf resumeStack;
    void *args;
    clock_t waitStart;
};

struct generic_context {
    struct context core;
    int unused[CTX_SIZE - sizeof(struct context)];
};

typedef enum task_status (*coroutine)(struct generic_context *ctx);

#define MAKE_COROUTINE(name, context_t, body)                                  \
    void name(context_t *ctx) {                                                \
        YIELD();                                                               \
        body;                                                                  \
    }

#define YIELD()                                                                \
    {                                                                          \
        int sig;                                                               \
        if ((sig = setjmp(((struct context *)ctx)->resumePoint)) == 0) {       \
            longjmp(((struct context *)ctx)->caller,                           \
                    kYeilding);                                \
        }                                                                      \
    }

#define KILL                                                                   \
    {                                                                          \
        int sig = setjmp(((struct context *)ctx)->resumePoint);                \
        (void)sig;                                                             \
        longjmp(((struct context *)ctx)->caller, COROUTINE_STATUS_DONE);       \
    }

void YIELD_WAITFORMILLIS(void * ctx, int ms) {
    int sig;
    ((struct context *)ctx)->waitStart = clock();
    if ((sig = setjmp(((struct context *)ctx)->resumeStack)) == 0) {
        longjmp(((struct context *)ctx)->caller, kYeilding);
    }
    if ((clock() - ((struct context *)ctx)->waitStart) * 1000 / CLOCKS_PER_SEC <
        ms) {
        longjmp(((struct context *)ctx)->caller, kYeilding);
    }
}

#define YIELD_WAITFORSECONDS(s) YIELD_WAITFORMILLIS(s * 1000)

struct task {
    // coroutine func;
    int running;
    enum task_status status;
    struct generic_context ctx;
};

static struct task tasks[MAX_TASKS];

int add_task(coroutine func, void *args) {
    for (int i = 0; i < MAX_TASKS; ++i) {
        if (tasks[i].running == 0) {
            tasks[i].running = 1;
            tasks[i].status = kYeilding;
            tasks[i].ctx = (struct generic_context){
                .core = {.args = args, .waitStart = clock()}};
            if (0 == setjmp(tasks[i].ctx.core.caller)) {
                func(&tasks[i].ctx);
            }
            return i;
        }
    }
    return 0;
}

void remove_task(int i) { tasks[i].running = 0; }

enum task_status runTask(int i) {
    int ret;
    if ((ret = setjmp(tasks[i].ctx.core.caller)) == 0) {
        // fprintf(stderr, "RES: %p\n", tasks[i].ctx.core.resumePoint);
        longjmp(tasks[i].ctx.core.resumeStack, 1);
    }
    return ret;
}

void runTasks() {
    for (int i = 0; i < MAX_TASKS; ++i) {
        if (tasks[i].running) {
            tasks[i].status = runTask(i);
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

void nats(struct print_ctx *ctx) {
    YIELD();
    ctx->user.ch = ctx->core.args;
    for (ctx->user.c = 0; ctx->user.c < 10; ++ctx->user.c) {
        send(int)(ctx->user.ch, ctx->user.c);
        YIELD_WAITFORMILLIS(ctx, 250);
    }
    close(ctx->user.ch);
    KILL;
}

#define LOCAL ctx->user

struct kernal_context {
    struct context core;
    struct {
        channel(int) c;
        channel(int) c2;
    } user;
};

void kernal(struct kernal_context *ctx) {
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
        if (closed(&LOCAL.c) && closed(&LOCAL.c2)) {
            KILL;
        }
        YIELD();
        // printf("ctx: %p\n", ctx);
    }
}

#define SCHEDULER for (; getStatus(0) != COROUTINE_STATUS_DONE; runTasks())
#define USE_EOS                                                                \
    int main() {                                                               \
        add_task((coroutine)kernal, NULL);                                     \
        SCHEDULER;                                                             \
    }

int main() {
    add_task((coroutine)kernal, NULL);
    SCHEDULER;
}

// WAIT - yeild till task is done + reaping
// WAITPID - that behavior