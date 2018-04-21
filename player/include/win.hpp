#ifndef WIN_HPP
#define WIN_HPP

#include <Windows.h>
#include <Psapi.h>
#include <string>
#include <set>

using FindWindowResult = std::set<HWND>;

template <class Predicate>
struct FindWindowContext {
    Predicate predicate;
    FindWindowResult matches;
};

template <class Pred>
BOOL on_window(HWND handle, LPARAM erased_context)
{
    auto context = reinterpret_cast<FindWindowContext<Pred> *>(erased_context);

    if (context->predicate(handle))
        context->matches.insert(handle);

    return TRUE;
}

template <class Pred>
FindWindowResult find_windows_by(Pred pred) {
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
    DWORD pid;
    GetWindowThreadProcessId(handle, &pid);
    auto process_flags = PROCESS_QUERY_INFORMATION | PROCESS_VM_READ;
    if (auto ph = OpenProcess(process_flags, FALSE, pid)) {
        char buff[256];
        auto size = GetModuleFileNameExA(ph, nullptr, buff, sizeof buff);
        return { buff, static_cast<std::string::size_type>(size) };
    } else {
        return { };
    }
}

FindWindowResult find_windows_by_title_and_pname(
    std::string title, std::string pname
)
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

#endif // WIN_HPP
