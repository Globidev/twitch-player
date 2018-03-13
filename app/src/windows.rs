extern crate winapi;

use self::winapi::shared::windef::HWND;
use self::winapi::shared::minwindef::{BOOL, LPARAM, DWORD, LPDWORD, FALSE, TRUE};
use self::winapi::um::winuser::{EnumWindows, GetWindowTextW, GetWindowThreadProcessId};
use self::winapi::ctypes::c_int;

use std::slice::from_raw_parts;

struct FindWindowContext<F: Fn(HWND) -> bool> {
    handle: Option<HWND>,
    predicate: F
}

extern "system" fn enum_window_callback<F>(handle: HWND, lparam: LPARAM) -> BOOL 
where 
    F: Fn(HWND) -> bool
{
    let context_ptr = lparam as *mut FindWindowContext<F>;

    if let Some(ref mut context) = unsafe { context_ptr.as_mut() } {
        if (context.predicate)(handle) {
            context.handle = Some(handle);
            return FALSE;
        }
    }

    TRUE
}

pub fn find_window<F>(predicate: F) -> Option<HWND>
where
    F: Fn(HWND) -> bool,
{
    let mut context = FindWindowContext {
        handle: None,
        predicate: predicate
    };

    unsafe {
        EnumWindows(
            Some(enum_window_callback::<F>), 
            &mut context as *mut FindWindowContext<F> as LPARAM
        )
    };

    context.handle
}

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

pub fn find_window_by_name<F>(name_predicate: F) -> Option<HWND>
where
    F: Fn(String) -> bool,
{
    let predicate = |handle: HWND| {
        get_window_text(handle)
            .map(|name| name_predicate(name))
            .unwrap_or(false)
    };

    find_window(predicate)
}

fn get_window_pid(handle: HWND) -> Option<DWORD> {
    let mut win_pid: DWORD = 0;
    unsafe { GetWindowThreadProcessId(handle, &mut win_pid as LPDWORD) };

    match win_pid {
        0 => None,
        pid => Some(pid)
    }
}

pub fn find_window_by_pid<F>(pid_predicate: F) -> Option<HWND> 
where
    F: Fn(u32) -> bool,
{
    let predicate = |handle: HWND| {
        get_window_pid(handle)
            .map(|pid| pid_predicate(pid))
            .unwrap_or(false)
    };

    find_window(predicate)
}
