TARGET          =   twitch-player
TEMPLATE        =   app

QT              +=  core gui widgets

QMAKE_CXXFLAGS  +=  /std:c++latest

SOURCES         +=  src/main.cpp \
                    src/main_window.cpp \
                    src/video_widget.cpp \
                    src/foreign_widget.cpp \
                    src/stream_widget.cpp \
                    src/libvlc.cpp

HEADERS         +=  include/main_window.hpp \
                    include/video_widget.hpp \
                    include/foreign_widget.hpp \
                    include/stream_widget.hpp \
                    include/libvlc.hpp \
                    include/constants.hpp

FORMS           +=  forms/main_window.ui
RESOURCES       =   build/player.qrc

INCLUDEPATH     +=  "C:\Users\depar\dev\vlc-3.0.1\include" \
                    include

LIBS            +=  -L"C:\Program Files\VideoLAN\VLC" \
                    -llibvlc \
                    -luser32

release:DESTDIR = release
debug:DESTDIR   = debug

OBJECTS_DIR     = $${DESTDIR}/.obj
MOC_DIR         = $${DESTDIR}/.moc
UI_DIR          = $${DESTDIR}/.ui

win32:RC_ICONS  += build/icon.ico
