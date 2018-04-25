#ifdef _WIN32
    #include "win32.hpp"
#elif __APPLE__
    static_assert("OSX not supported yet")
#elif __linux__
    static_assert("Linux not supported yet")
#else
    static_assert("Unsupported target")
#endif
