#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]

use crate::root as coco;
use std::ffi::*;
include!(concat!(env!("OUT_DIR"), "/bindings.rs"));

type coroutine = extern "C" fn(*mut libc::c_void);

fn getContext(tid: i32) -> &'static mut coco::context {
    unsafe { coco::getContext(tid).as_mut().unwrap() }
}

fn coco_start(kernal: coroutine) {
    unsafe { coco::coco_start(Some(kernal), core::ptr::null_mut()) }
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

fn take_args<T>(arg: *mut libc::c_void) -> T {
    let raw_args: *mut T = arg.cast();
    match raw_args {
        _ if raw_args == (std::ptr::null_mut() as *mut T) => panic!("no args"),
        _ => unsafe { *Box::from_raw(raw_args) },
    }
}

fn see_args<'a, T>(arg: *mut libc::c_void) -> &'a T {
    let raw_args: *mut T = arg.cast() ;
    unsafe { raw_args.as_ref().expect("no args") }
}

fn coco_waitpid(tid: c_int, exit_status: Option<&mut i32>, options: c_uint) -> c_int {
    let in_es = match exit_status {
        Some(r) => r as *mut i32,
        None => std::ptr::null_mut(),
    };
    unsafe { coco::coco_waitpid(tid, in_es, options as c_int) }
}

extern "C" fn test(delay: *mut libc::c_void) {
    let delay = take_args::<u32>(delay);
    for i in 0..10 {
        println!("{} {:?}", i, delay);
        yield_for_ms(delay);
    }
    coco_exit(0);
}

extern "C" fn kernal(_: *mut libc::c_void) {
    let tid0 = add_task(test, Some(250));
    let tid1 = add_task(test, Some(400));
    while coco_waitpid(tid0, None, coco::COCO_WNOHANG) == 0 {coco_yield()}
    coco_waitpid(tid1, None, 0);
    coco_exit(0);
}

#[test]
fn main() {
    println!("Hello, world!");
    coco_start(kernal);
}
