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
