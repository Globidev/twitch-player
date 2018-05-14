#pragma once

#ifdef _WIN32
    #include "ui/native/win32.hpp"
#elif __APPLE__
    static_assert(false, "OSX not supported yet");
#elif __linux__
    static_assert(false, "Linux not supported yet");
#else
    static_assert(false, "Unsupported target");
#endif
