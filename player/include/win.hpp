//#include <Windows.h>

//struct Handles {
//    HWND chat, vlc;
//};

//BOOL on_window(HWND handle, LPARAM l) {
//    auto *handles = reinterpret_cast<Handles *>(l);

//    char buff[256];
//    auto size = GetWindowTextA(handle, buff, 256);

//    std::string s(buff, size);
//    if (s.compare(0, 6, "Twitch") == 0) {
//        handles->chat = handle;
//    }
//    if (s.compare(0, 9, "input.avi") == 0) {
//        handles->vlc= handle;
//    }

//    return TRUE;
//}
