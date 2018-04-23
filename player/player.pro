TARGET          =   twitch-player
TEMPLATE        =   app

QT              +=  core gui widgets

QMAKE_CXXFLAGS  +=  /std:c++latest

SOURCES         +=  src/main.cpp \
                    src/main_window.cpp \
                    src/libvlc.cpp \
                    src/widgets/video_widget.cpp \
                    src/widgets/foreign_widget.cpp \
                    src/widgets/stream_widget.cpp \
                    src/widgets/stream_picker.cpp \
                    src/widgets/stream_container.cpp \

HEADERS         +=  include/main_window.hpp \
                    include/constants.hpp \
                    include/libvlc.hpp \
                    include/widgets/video_widget.hpp \
                    include/widgets/foreign_widget.hpp \
                    include/widgets/stream_widget.hpp \
                    include/widgets/stream_picker.hpp \
                    include/widgets/stream_container.hpp \

FORMS           +=  forms/main_window.ui \
                    forms/stream_picker.ui
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
