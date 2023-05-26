#include <setjmp.h>
#include <time.h>

#include "vmac.h"

#define MAYBE_UNUSED(x) (void)x

#define MAX_TASKS 100
#define CTX_SIZE 8000

enum COROUTINE_STATUS {
    kDead,
    kDone,
    kYielding,
    kStopped,
    kNew,
};
struct context;

struct context *ctx;

typedef void (*coroutine)(void);
typedef void (*signalHandler)(void);

#define SIGNONE 0
#define SIGINT 1
#define SIGSTP 2

struct context {
    coroutine func;
    jmp_buf caller;
    jmp_buf resumePoint;
    int signal;
    void *args;
    signalHandler handlers[2];
    clock_t waitStart;
    int user[CTX_SIZE];
};

void doSignal(void);

#define MAKE_COROUTINE(name, context_t, body)                                  \
    void name(context_t *i_ctx) {                                              \
        yield();                                                               \
        body;                                                                  \
    }

#define yield()                                                                \
    {                                                                          \
        int signal;                                                            \
        if ((signal = setjmp(ctx->resumePoint)) == 0) {                        \
            longjmp(ctx->caller, kYielding);                                   \
        }                                                                      \
        doSignal();                                                            \
    }

#define yieldStatus(stat)                                                      \
    {                                                                          \
        int signal;                                                            \
        if ((signal = setjmp(ctx->resumePoint)) == 0) {                        \
            longjmp(ctx->caller, stat);                                        \
        }                                                                      \
        doSignal();                                                            \
    }

#define KILL                                                                   \
    {                                                                          \
        int signal;                                                            \
        signal = setjmp(ctx->resumePoint);                                   \
        longjmp(ctx->caller, kDone);                                           \
    }

#define YIELD_WAITFORMILLIS(ms)                                                \
    {                                                                          \
        int signal;                                                            \
        ctx->waitStart = clock();                                              \
        if ((signal = setjmp(ctx->resumePoint)) == 0) {                        \
            longjmp(ctx->caller, kYielding);                                   \
        }                                                                      \
        doSignal();                                                            \
        if ((clock() - ctx->waitStart) * 1000 / CLOCKS_PER_SEC < ms) {         \
            longjmp(ctx->caller, kYielding);                                   \
        }                                                                      \
    }

#define YIELD_WAITFORSECONDS(s) YIELD_WAITFORMILLIS(s * 1000)

int add_task(coroutine func, void *args);
void remove_task(int i);
enum COROUTINE_STATUS runTask(int i);
void runTasks();
enum COROUTINE_STATUS getStatus(int tid);
struct context *getContext(int tid);

#define WAIT_OPT_NOHANG (1 << 0)
int waitpid(int tid, int opts);

#define DERIVE_CONTEXT                                                         \
    struct context core;                                                       \
    struct {
#define END_CONTEXT                                                            \
    }                                                                          \
    user;

#define LOCAL(type) ((type *)(&ctx->user))
#define CORE ctx

// #define CALL_WITH_YIELD_ABILITY(name, ...) name((void *)&ctx, ##__VA_ARGS__)