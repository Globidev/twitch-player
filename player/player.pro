TARGET          =   twitch-player
TEMPLATE        =   app

QT              +=  core gui widgets

win32:QMAKE_CXXFLAGS    +=  /std:c++latest

release:CONFIG  +=  static

SOURCES         +=  src/main.cpp \
                    src/main_window.cpp \
                    src/libvlc.cpp \
                    src/widgets/video_widget.cpp \
                    src/widgets/foreign_widget.cpp \
                    src/widgets/stream_widget.cpp \
                    src/widgets/stream_picker.cpp \
                    src/widgets/stream_container.cpp \
                    src/widgets/vlc_log_viewer.cpp \
                    src/widgets/options_dialog.cpp \

win32:SOURCES   +=  src/native/win32.cpp

HEADERS         +=  include/main_window.hpp \
                    include/constants.hpp \
                    include/libvlc.hpp \
                    include/widgets/video_widget.hpp \
                    include/widgets/foreign_widget.hpp \
                    include/widgets/stream_widget.hpp \
                    include/widgets/stream_picker.hpp \
                    include/widgets/stream_container.hpp \
                    include/widgets/vlc_log_viewer.hpp \
                    include/widgets/options_dialog.hpp \
                    include/native/capabilities.hpp \

win32:HEADERS   +=  include/native/win32.hpp

FORMS           +=  forms/main_window.ui \
                    forms/stream_picker.ui \
                    forms/vlc_log_viewer.ui \
                    forms/options_dialog.ui

RESOURCES       =   resources/player.qrc

LIBVLC_INCLUDE_DIR  =   $$(LIBVLC_INCLUDE_DIR)
LIBVLC_LIB_DIR      =   $$(LIBVLC_LIB_DIR)
isEmpty(LIBVLC_INCLUDE_DIR) {
    warning("the LIBVLC_INCLUDE_DIR variable is not set!")
}
isEmpty(LIBVLC_LIB_DIR) {
    warning("the LIBVLC_LIB_DIR variable is not set!")
}

INCLUDEPATH     +=  include \
                    $${LIBVLC_INCLUDE_DIR}

LIBS            +=  -L$${LIBVLC_LIB_DIR} -llibvlc
win32:LIBS      +=  -luser32

release:DESTDIR = release
debug:DESTDIR   = debug

MOC_DIR         = $${DESTDIR}/.moc
OBJECTS_DIR     = $${DESTDIR}/.obj
RCC_DIR         = $${DESTDIR}/.rcc
UI_DIR          = $${DESTDIR}/.ui

win32:RC_ICONS  += resources/icon.ico
