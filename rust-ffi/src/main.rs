#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]

use crate::root as coco;
use std::ffi::*;
include!(concat!(env!("OUT_DIR"), "/bindings.rs"));

type coroutine = extern "C" fn();

fn getContext(tid: i32) -> &'static mut coco::context {
    unsafe { coco::getContext(tid).as_mut().unwrap() }
}

fn coco_start(kernal: coroutine) {
    unsafe { coco::coco_start(Some(kernal)) }
}
fn coco_exit(i: c_uint) {
    unsafe { coco::coco_exit(i) }
}

fn coco_yield() {
    unsafe{coco::coco_yield()}
}

fn yield_for_s(i: c_uint) {
    unsafe { coco::yieldForS(i) }
}

fn yield_for_ms(i: c_uint) {
    unsafe { coco::yieldForMs(i) }
}

fn add_task<T>(task: coroutine, args: Option<T>) -> c_int {
    let in_args = match args {
        Some(r) => (Box::into_raw(Box::new(r))) as *mut c_void,
        None => std::ptr::null_mut(),
    };
    unsafe { coco::add_task(Some(task), in_args) }
}

fn take_args<T>() -> T {
    let raw_args: *mut T = unsafe { coco::ctx.read_unaligned().args.cast() };
    match raw_args {
        _ if raw_args == (std::ptr::null_mut() as *mut T) => panic!("no args"),
        _ => unsafe { *Box::from_raw(raw_args) },
    }
}

fn see_args<'a, T>() -> &'a T {
    let _nul: *mut T = std::ptr::null_mut();
    let raw_args: *mut T = unsafe { coco::ctx.read_unaligned().args.cast() };
    unsafe { raw_args.as_ref().expect("no args") }
}

fn coco_waitpid(tid: c_int, exit_status: Option<&mut i32>, options: c_uint) -> c_int {
    let in_es = match exit_status {
        Some(r) => r as *mut i32,
        None => std::ptr::null_mut(),
    };
    unsafe { coco::coco_waitpid(tid, in_es, options as c_int) }
}

extern "C" fn test() {
    let delay = take_args::<u32>();
    for i in 0..10 {
        println!("{} {:?}", i, delay);
        yield_for_ms(delay);
    }
    coco_exit(0);
}

extern "C" fn kernal() {
    let tid0 = add_task(test, Some(250));
    let tid1 = add_task(test, Some(400));
    while coco_waitpid(tid0, None, coco::COCO_WNOHANG) == 0 {coco_yield()}
    coco_waitpid(tid1, None, 0);
    coco_exit(0);
}

fn main() {
    println!("Hello, world!");
    coco_start(kernal);
}
