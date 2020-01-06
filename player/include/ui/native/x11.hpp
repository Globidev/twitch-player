#pragma once

#include <string>
#include <set>
#include <regex>

using WindowHandle = unsigned long;
using FindWindowResult = std::set<WindowHandle>;

FindWindowResult find_windows_by_title_and_pname(std::regex, std::string);

void toggle_window_borders(WindowHandle, bool);

void toggle_always_on_top(WindowHandle, bool);

void sysclose_window(WindowHandle);

void redraw(WindowHandle);

void set_transparent(WindowHandle);
