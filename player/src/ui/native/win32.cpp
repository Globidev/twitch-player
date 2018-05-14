#include "ui/native/win32.hpp"

#include <Windows.h>
#include <Psapi.h>

template <class Predicate>
struct FindWindowContext {
    Predicate predicate;
    FindWindowResult matches;
};

template <class Pred>
static BOOL on_window(HWND handle, LPARAM erased_context)
{
    auto context = reinterpret_cast<FindWindowContext<Pred> *>(erased_context);

    if (context->predicate(handle))
        context->matches.insert(handle);

    return TRUE;
}

template <class Pred>
static FindWindowResult find_windows_by(Pred pred) {
    auto context = FindWindowContext<Pred> { pred };

    (void)EnumWindows(on_window<Pred>, reinterpret_cast<LPARAM>(&context));

    return context.matches;
}

static std::string window_title(HWND handle) {
    char buff[256];
    auto size = GetWindowTextA(handle, buff, sizeof buff);
    return { buff, static_cast<std::string::size_type>(size) };
}

static std::string window_process_name(HWND handle) {
    constexpr auto PROCESS_FLAGS = PROCESS_QUERY_INFORMATION | PROCESS_VM_READ;

    DWORD pid;
    GetWindowThreadProcessId(handle, &pid);

    if (auto phandle = OpenProcess(PROCESS_FLAGS, FALSE, pid); phandle) {
        char buff[256];
        auto size = GetModuleFileNameExA(phandle, nullptr, buff, sizeof buff);
        return { buff, static_cast<std::string::size_type>(size) };
    } else {
        return { };
    }
}

FindWindowResult find_windows_by_title_and_pname(std::string title,
                                                 std::string pname)
{
    auto predicate = [&](HWND handle) {
        auto handle_title = window_title(handle);
        auto handle_pname = window_process_name(handle);
        // no starts_with until c++20...
        return (
            handle_title.compare(0, title.size(), title) == 0 &&
            handle_pname.compare(0, pname.size(), pname) == 0
        );
    };

    return find_windows_by(predicate);
}

void toggle_window_borders(HWND handle, bool on) {
    constexpr auto BORDER_FLAGS = WS_CAPTION
                                | WS_THICKFRAME
                                | WS_MINIMIZEBOX
                                | WS_MAXIMIZEBOX
                                | WS_SYSMENU
                                ;

    constexpr auto REPOSITION_FLAGS = SWP_DRAWFRAME
                                    | SWP_FRAMECHANGED
                                    | SWP_NOMOVE
                                    | SWP_NOSIZE
                                    | SWP_NOZORDER
                                    | SWP_NOACTIVATE
                                    ;

    auto current_style = GetWindowLong(handle, GWL_STYLE);
    auto new_style = on
                   ? current_style | BORDER_FLAGS
                   : current_style & ~BORDER_FLAGS;
    SetWindowLong(handle, GWL_STYLE, new_style);
    SetWindowPos(handle, nullptr, 0, 0, 0, 0, REPOSITION_FLAGS);
    redraw(handle);
}

void toggle_always_on_top(HWND handle, bool on) {
    auto insert_flags = on ? HWND_TOPMOST : HWND_NOTOPMOST;
    SetWindowPos(handle, insert_flags, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
}

void sysclose_window(HWND handle) {
    SendMessage(handle, WM_SYSCOMMAND, SC_CLOSE, 0);
}

void redraw(HWND handle) {
    InvalidateRect(handle, nullptr, TRUE);
}
