#include <stdio.h>
#include <time.h>

enum task_status {
    COROUTINE_STATUS_DONE = 0,
    COROUTINE_STATUS_VALUE = 1,
    kYeilding = 2,

};

#define MAYBE_UNUSED(x) (void)x

#define MAKE_COROUTINE(ret_t, name, body)                                      \
    static ret_t name##_out;                                                   \
    enum COROUTINE_STATUS name(void *init_args, ret_t *ret) {                  \
        static int resumePoint = -1;                                           \
        static void *args;                                                     \
        MAYBE_UNUSED(args);                                                    \
        switch (resumePoint) {                                                 \
        case -1:                                                               \
            args = init_args;                                                  \
            YIELD(0);                                                          \
            body;                                                              \
        case -2:                                                               \
        default:                                                               \
            resumePoint = -2;                                                  \
        }                                                                      \
        return COROUTINE_STATUS_DONE;                                          \
    }

#define YIELD(i)                                                               \
    resumePoint = i;                                                           \
    return kYeilding;                                           \
    case i:

#define YIELD_RETURN(i, r)                                                     \
    resumePoint = i;                                                           \
    *ret = r;                                                                  \
    return COROUTINE_STATUS_VALUE;                                             \
    case i:

#define STOP                                                                   \
    resumePoint = __INT_MAX__;                                                 \
    case __INT_MAX__:                                                          \
        return COROUTINE_STATUS_DONE;

#define YIELD_WAITFORMILIS(i, ms)                                              \
    resumePoint = i;                                                           \
    static clock_t start;                                                      \
    start = clock();                                                           \
    case i:                                                                    \
        if ((clock() - start) * 1000 / CLOCKS_PER_SEC < ms) {                  \
            return kYeilding;                                   \
        }

#define YIELD_WAITFORSECONDS(i, s) YIELD_WAITFORMILIS(i, s * 1000)

#define co static

#define COROUTINE_INIT(name, in)                                               \
    enum COROUTINE_STATUS name##_status = COROUTINE_STATUS_VALUE;              \
    typeof(in) name##_indata = in;                                             \
    name##_status = name(&name##_indata, &name##_out);

#define COROUTINE_NEXT(name) (name##_status = name(NULL, &name##_out))
#define COROUTINE_IS_DONE(name) (name##_status == COROUTINE_STATUS_DONE)
#define COROUTINE_HAS_VALUE(name) (name##_status == COROUTINE_STATUS_VALUE)
#define COROUTINE_ON_VALUE(name) if (COROUTINE_HAS_VALUE(name))
#define COROUTINE_VALUE(name) name##_out

#define SCHEDULE1(x) for (; !COROUTINE_IS_DONE(x); COROUTINE_NEXT(x))
#define SCHEDULE2(x, y)                                                        \
    for (; !COROUTINE_IS_DONE(x) || !COROUTINE_IS_DONE(y);                     \
         COROUTINE_NEXT(x), COROUTINE_NEXT(y))
#define SCHEDULE3(x, y, z)                                                     \
    for (; !COROUTINE_IS_DONE(x) || !COROUTINE_IS_DONE(y) ||                   \
           !COROUTINE_IS_DONE(z);                                              \
         COROUTINE_NEXT(x), COROUTINE_NEXT(y), COROUTINE_NEXT(z))
#define SCHEDULE4(a, x, y, z)                                                  \
    for (; !COROUTINE_IS_DONE(x) || !COROUTINE_IS_DONE(y) ||                   \
           !COROUTINE_IS_DONE(z) || !COROUTINE_IS_DONE(a);                     \
         COROUTINE_NEXT(x), COROUTINE_NEXT(y), COROUTINE_NEXT(z),              \
         COROUTINE_NEXT(a))
#define SCHEDULE5(a, b, x, y, z)                                               \
    for (; !COROUTINE_IS_DONE(x) || !COROUTINE_IS_DONE(y) ||                   \
           !COROUTINE_IS_DONE(z) || !COROUTINE_IS_DONE(a) ||                   \
           !COROUTINE_IS_DONE(b);                                              \
         COROUTINE_NEXT(x), COROUTINE_NEXT(y), COROUTINE_NEXT(z),              \
         COROUTINE_NEXT(a), COROUTINE_NEXT(b))

#define VA_NUM_ARGS_IMPL(_1, _2, _3, _4, _5, N, ...) N
#define VA_NUM_ARGS(...) VA_NUM_ARGS_IMPL(__VA_ARGS__, 5, 4, 3, 2, 1)

#define SCHEDULE_IMPL_(nargs) SCHEDULE##nargs
#define SCHEDULE_IMPL(nargs) SCHEDULE_IMPL_(nargs)
#define SCHEDULE(...) SCHEDULE_IMPL(VA_NUM_ARGS(__VA_ARGS__))(__VA_ARGS__)

MAKE_COROUTINE(int, nats, {
    co int c = 0;
    for (; c < 10;) {
        YIELD_RETURN(1, ++c);
        YIELD_WAITFORMILLIS(2, 500);
    }
    KILL;
})

char buf[100];

MAKE_COROUTINE(char *, strings, {
    co int c = 0;
    for (; c < 10;) {
        sprintf(buf, "string %d", ++c);
        YIELD_RETURN(1, buf);
        YIELD_WAITFORMILLIS(2, 1000);
    }
    KILL;
})

MAKE_COROUTINE(int, multiples, {
    co int c;
    co int m;
    c = 0;
    m = *(int *)args;
    for (; c < 10;) {
        YIELD_RETURN(1, m * ++c);
        YIELD_WAITFORMILLIS(2, 2000);
    }
    KILL;
})

int main() {
    COROUTINE_INIT(nats, NULL);
    COROUTINE_INIT(strings, NULL);
    COROUTINE_INIT(multiples, 10);
    SCHEDULE(nats, strings, multiples) {
        if (COROUTINE_HAS_VALUE(nats))
            printf("nats:      %d\n", COROUTINE_VALUE(nats));
        if (COROUTINE_HAS_VALUE(strings))
            printf("strings:   %s\n", COROUTINE_VALUE(strings));
        if (COROUTINE_HAS_VALUE(multiples))
            printf("multiples: %d\n", COROUTINE_VALUE(multiples));
    }
    return 0;
}