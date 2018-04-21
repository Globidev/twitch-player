extern crate x11;

use self::x11::xlib;
use self::xlib::Display;

use std::ptr;
use std::os::raw::c_void;
use std::ffi::CStr;

use std::collections::VecDeque;

pub type Handle = xlib::Window;

pub fn find_window<F>(predicate: F) -> Option<Handle>
where
    F: Fn(*mut Display, Handle) -> bool,
{
    use std::slice::from_raw_parts;

    let display = unsafe { xlib::XOpenDisplay(ptr::null()) };
    let root_window = unsafe { xlib::XRootWindow(display, 0) };

    let mut _root = 0u64;
    let mut _parent = 0u64;
    let mut c_children = ptr::null_mut();
    let mut children_count = 0u32;

    let mut open_set = VecDeque::new();
    open_set.push_back(root_window);

    while !open_set.is_empty() {
        let win = open_set.pop_front().unwrap();

        unsafe {
            xlib::XQueryTree(
                display,
                win,
                &mut _root as *mut Handle,
                &mut _parent as *mut Handle,
                &mut c_children as *mut *mut Handle,
                &mut children_count as *mut u32
            );
        }

        let children = unsafe { from_raw_parts(c_children, children_count as usize) };

        for child in children {
            // println!("WIN: {:?} {}", get_window_text(display, *child), predicate(display, *child));
            if predicate(display, *child) {
                unsafe { xlib::XCloseDisplay(display) };
                return Some(*child)
            }
            open_set.push_back(*child)
        }
        unsafe { xlib::XFree(c_children as *mut c_void); }
    }

    unsafe { xlib::XCloseDisplay(display) };
    None
}

fn get_window_text(display: *mut Display, handle: Handle) -> Option<String> {
    // let mut name = ptr::null_mut();
    let mut text = xlib::XTextProperty {
        value: ptr::null_mut(),
        encoding: 0,
        format: 0,
        nitems: 0
    };

    unsafe {
        // println!("{:?}", xlib::XFetchName(display, handle, &mut name as *mut *mut i8));
        // if name == ptr::null_mut() {
        //     None
        // } else {
        // println!("{:?}", CStr::from_ptr(name)
        //     .to_str()
        //     .ok().map(|s| s.to_string()));
        //     CStr::from_ptr(name)
        //         .to_str()
        //         .ok()
        //         .map(From::from)
        // }

        xlib::XGetTextProperty(display, handle, &mut text as *mut xlib::XTextProperty, xlib::XA_WM_NAME);
        use std::slice::from_raw_parts;
        use std::str::from_utf8_unchecked;
        if text.nitems > 0 {
            let as_slice = from_raw_parts(text.value, text.nitems as usize);
            let s = from_utf8_unchecked(&as_slice);
            // println!("{}", s);
            Some(s.to_string())
        }
        else {
            None
        }


    }
}

pub fn find_window_by_name<F>(name_predicate: F) -> Option<Handle>
where
    F: Fn(String) -> bool,
{
    let predicate = |display, handle: Handle| {
        get_window_text(display, handle)
            .map(|name| name_predicate(name))
            .unwrap_or(false)
    };

    find_window(predicate)
}

pub fn find_window_by_pid<F>(pid_predicate: F) -> Option<Handle>
where
    F: Fn(u32) -> bool,
{
    // let predicate = |handle: Handle| {
    //     get_window_pid(handle)
    //         .map(|pid| pid_predicate(pid))
    //         .unwrap_or(false)
    // };

    // find_window(predicate)
    None
}
