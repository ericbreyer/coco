use std::{ffi::*, mem::size_of};

type Coroutine = extern "C" fn();
type SignalHandler = Coroutine;
type JmpBuf = [c_int; 37];

#[repr(C)]
struct context {
    caller : JmpBuf,
    resume_point: JmpBuf,
    sig_bits: c_longlong,
    handlers : [SignalHandler; 3],
    wait_start: c_longlong,
    exit_status: c_int,
    args: *mut c_int,
    saved_frame: [c_char; 1 << 11],
    frame_start: *mut c_int,
    frame_size: c_longlong
}

// jmp_buf caller;        ///< the jump point to yield control to
// jmp_buf resumePoint;   ///< the index of the next resume point to pop
// long long int sigBits; ///< a bit vector of signals
// signalHandler handlers[NUM_SIGNALS]; ///< handler callbacks for said signals
// // Yielding Data
// clock_t waitStart;
// // Exit Status
// int exitStatus;
// // User data
// void *args;
// char savedFrame[USR_CTX_SIZE];
// void *frameStart;
// ptrdiff_t frameSize;

// struct context *getContext(int tid);

extern "C" {
    static ctx : *mut context;
}

fn getContext(tid : i32) -> &'static mut context {
    extern "C" {
        fn getContext(tid : c_int) -> *mut context;
    }
    unsafe {
        println!("---- {}", (*getContext(tid)).frame_size);
        getContext(tid).as_mut().unwrap()
    }
}

fn get_context() -> &'static mut context {
    unsafe{
        println!("---- {}", (*ctx).frame_size);
        ctx.as_mut().unwrap()
    }
}
fn coco_start(kernal: Coroutine) {
    extern "C" {
        fn coco_start(kernal: Coroutine);
    }
    unsafe { coco_start(kernal) }
}
fn coco_exit(i: c_uint) {
    extern "C" {
        fn coco_exit(i: c_uint);
    }
    unsafe { coco_exit(i) }
}
fn yield_for_s(i: c_uint) {
    extern "C" {
        fn yieldForS(i: c_uint);
    }    
    unsafe { yieldForS(i) }
}
fn add_task<T>(task: Coroutine, args: Option<Box<T>>) -> c_int {
    extern "C" {
        fn add_task(task: Coroutine, args: *mut c_void) -> c_int;
    }    
    unsafe {
        let in_args = match args {
            Some(r) => (r.as_ref() as *const T) as *mut c_void,
            None => std::ptr::null_mut(),
        };
        add_task(task, in_args)
    }
}
fn coco_waitpid(tid: c_int, exit_status: Option<&mut i32>, options: c_int) -> c_int {
    extern "C" {
        fn coco_waitpid(tid: c_int, exitStatus: *mut c_int, options: c_int) -> c_int;
    }
    unsafe {
        let in_es = match exit_status {
            Some(r) => r as *mut i32,
            None => std::ptr::null_mut(),
        };
        coco_waitpid(tid, in_es, options)
    }
}

extern "C" fn test() {
    for i in 0..10 {
        println!("{} {:?}", i, get_context().frame_size);
        yield_for_s(1);
    }
    coco_exit(0);
}

extern "C" fn kernal() {
    let tid0 = add_task::<c_void>(test, None);
    let tid1 = add_task::<c_void>(test, None);
    coco_waitpid(tid0, None, 0);
    coco_waitpid(tid1, None, 0);
    coco_exit(0);
}

fn main() {
    println!("Hello, world!");
    coco_start(kernal);
}