#include "ui/native/osx.hpp"

FindWindowResult find_windows_by_title_and_pname(std::string, std::string) {
    return { };
}

void toggle_window_borders(WindowHandle, bool) { }

void toggle_always_on_top(WindowHandle, bool) { }

void sysclose_window(WindowHandle) { }

void redraw(WindowHandle) { }

void set_transparent(WindowHandle) { }
