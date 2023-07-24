use std::{ffi::*, mem::size_of, fmt::Debug};

type Coroutine = extern "C" fn();
type SignalHandler = Coroutine;
type JmpBuf = [c_int; 48];

#[repr(C, packed)]
struct context {
    caller : JmpBuf,
    resume_point: JmpBuf,
    sig_bits: c_longlong,
    handlers : [SignalHandler; 3],
    wait_start: c_longlong,
    exit_status: c_int,
    args: *mut c_int,
    saved_frame: [c_char; 1 << 9],
    frame_start: *mut c_int,
    frame_size: c_longlong
}

#[link(name = "coco", kind = "static")]
extern "C" {
    static mut ctx : *mut context;
}

fn getContext(tid : i32) -> &'static mut context {
    extern "C" {
        fn getContext(tid : c_int) -> *mut context;
    }
    unsafe {
        println!("---- {}", size_of::<context>());
        getContext(tid).as_mut().unwrap()
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

fn yield_for_ms(i: c_uint) {
    extern "C" {
        fn yieldForMs(i: c_uint);
    }    
    unsafe { yieldForMs(i) }
}

fn add_task<T>(task: Coroutine, args: Option<Box<T>>) -> c_int {
    extern "C" {
        fn add_task(task: Coroutine, args: *mut c_void) -> c_int;
    }    
    unsafe {
        let in_args = match args {
            Some(r) => (Box::into_raw(r)) as *mut c_void,
            None => std::ptr::null_mut(),
        };
        add_task(task, in_args)
    }
}

fn take_args<T>() -> Box<T> {
    unsafe {
        let nul : *mut T = std::ptr::null_mut();
        let raw_args: *mut T = ctx.read_unaligned().args.cast();
        match raw_args {
            _ if raw_args == nul  => panic!("no args"),
            _ => Box::from_raw(raw_args)
        }
    }
}

fn see_args<'a, T>() -> &'a T {
    unsafe {
        let nul : *mut T = std::ptr::null_mut();
        let raw_args: *mut T = ctx.read_unaligned().args.cast();
        raw_args.as_ref().expect("no args")
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
    let delay = *(take_args::<u32>());
    for i in 0..10 {
        println!("{} {:?}", i, delay);
        yield_for_ms(delay);
    }
    coco_exit(0);
}

extern "C" fn kernal() {
    let tid0 = add_task(test, Some( Box::new(250)));
    let tid1 = add_task(test, Some( Box::new(400)));
    coco_waitpid(tid0, None, 0);
    coco_waitpid(tid1, None, 0);
    coco_exit(0);
}

fn main() {
    println!("Hello, world!");
    coco_start(kernal);
}