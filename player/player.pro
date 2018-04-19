TARGET          =   player
TEMPLATE        =   app

QT              +=  core gui widgets

QMAKE_CXXFLAGS  +=  /std:c++latest

SOURCES         +=  src/main.cpp \
                    src/main_window.cpp \
                    src/video_widget.cpp \
                    src/libvlc.cpp

HEADERS         +=  include/main_window.hpp \
                    include/video_widget.hpp \
                    include/libvlc.hpp \
                    include/constants.hpp

FORMS           +=  forms/main_window.ui

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
