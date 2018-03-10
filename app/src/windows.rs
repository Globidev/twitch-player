extern crate winapi;

use self::winapi::shared::windef::HWND;
use self::winapi::shared::minwindef::{BOOL, FALSE, LPARAM, TRUE};
use self::winapi::um::winuser::{EnumWindows, GetWindowTextW};
use self::winapi::ctypes::c_int;

use std::ptr;
use std::slice::from_raw_parts;

fn get_window_text(handle: HWND) -> Option<String> {
    let mut text_buffer = [0u16; 256];
    let length = unsafe { 
        GetWindowTextW(
            handle, 
            text_buffer.as_mut_ptr(), 
            text_buffer.len() as c_int
        ) 
    };

    if length != 0 {
        let exact_text = unsafe { 
            from_raw_parts(text_buffer.as_ptr(), length as usize) 
        };
        Some(String::from_utf16_lossy(exact_text))
    } else {
        None
    }
}

struct FindWindowContext<F: FnMut(&str) -> bool> {
    handle: HWND,
    predicate: F,
}

extern "system" fn on_window<F>(handle: HWND, lparam: LPARAM) -> BOOL 
where 
    F: FnMut(&str) -> bool
{
    let context_ptr = lparam as *mut FindWindowContext<F>;

    if let Some(title) = get_window_text(handle) {
        if let Some(ref mut context) = unsafe { context_ptr.as_mut() } {
            if (context.predicate)(&title) {
                context.handle = handle;
                return FALSE
            }
        }
    }

    TRUE
}

pub fn find_window_by_name<F>(pred: F) -> Option<HWND>
where
    F: FnMut(&str) -> bool,
{
    let mut context = FindWindowContext {
        handle: ptr::null_mut(),
        predicate: pred
    };

    unsafe {
        EnumWindows(
            Some(on_window::<F>), 
            &mut context as *mut FindWindowContext<F> as LPARAM
        )
    };

    match context.handle.is_null() {
        true  => None,
        false => Some(context.handle)
    }
}
