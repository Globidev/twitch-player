#ifndef WIN32_HPP
#define WIN32_HPP

#include <string>
#include <set>

struct HWND__;

using WindowHandle = HWND__ *;
using FindWindowResult = std::set<WindowHandle>;

FindWindowResult find_windows_by_title_and_pname(std::string, std::string);

void toggle_window_borders(WindowHandle);

void sysclose_window(WindowHandle);

#endif // WIN32_HPP