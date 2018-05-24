#pragma once

#ifdef _WIN32
    #include "ui/native/win32.hpp"
#elif __APPLE__
    #include "ui/native/osx.hpp"
#elif __linux__
    #include "ui/native/x11.hpp"
#else
    static_assert(false, "Unsupported target");
#endif

template <class To, class From>
static constexpr auto reify_handle(From from) {
    constexpr auto from_is_pointer = std::is_pointer_v<From>,
                   to_is_pointer = std::is_pointer_v<To>;

    if constexpr (from_is_pointer != to_is_pointer)
        return reinterpret_cast<To>(from);
    else
        return static_cast<To>(from);
}

static constexpr auto to_native_handle(uintptr_t qt_handle) {
    return reify_handle<WindowHandle>(qt_handle);
}

static constexpr auto from_native_handle(WindowHandle native_handle) {
    return reify_handle<uintptr_t>(native_handle);
}