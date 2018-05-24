#include "ui/native/x11.hpp"

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <deque>

FindWindowResult find_windows_by_title_and_pname(std::string title, std::string pname) {
    auto matches = FindWindowResult { };

    auto display = XOpenDisplay(nullptr);
    auto root_window = XRootWindow(display, 0);

    auto open_set = std::deque<Window> { root_window };

    while (!open_set.empty()) {
        auto window = open_set.front();
        open_set.pop_front();

        Window _root, _parent;
        Window *children;
        unsigned int child_count;
        XQueryTree(display, window, &_root, &_parent, &children, &child_count);

        for (auto i = 0u; i < child_count; ++i) {
            auto child = children[i];
            XTextProperty property;
            XGetWMName(display, child, &property);
            if (property.value) {
                if (title.compare(reinterpret_cast<char *>(property.value)) == 0)
                    matches.insert(child);
            }

            open_set.push_back(child);
        }
        XFree(children);
    }

    XCloseDisplay(display);
    return matches;
}

void toggle_window_borders(WindowHandle, bool) {
    // TODO
}

void toggle_always_on_top(WindowHandle, bool) {
    // TODO
}

void sysclose_window(WindowHandle) {
    // TODO
}

void redraw(WindowHandle) {
    // TODO
}

void set_transparent(WindowHandle) {
    // TODO
}
