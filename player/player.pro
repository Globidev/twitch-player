TARGET          =   twitch-player
TEMPLATE        =   app

QT              +=  core gui widgets network

include(vendor/qtpromise-0.3.0/qtpromise.pri)

win32:QMAKE_CXXFLAGS    +=  /std:c++latest

release:CONFIG  +=  static

SOURCES         +=  src/main.cpp \
                    src/main_window.cpp \
                    src/main_window_shortcuts.cpp \
                    src/libvlc.cpp \
                    src/vlc_logger.cpp \
                    src/vlc_event_watcher.cpp \
                    src/widgets/video_widget.cpp \
                    src/widgets/foreign_widget.cpp \
                    src/widgets/stream_widget.cpp \
                    src/widgets/stream_picker.cpp \
                    src/widgets/stream_container.cpp \
                    src/widgets/vlc_log_viewer.cpp \
                    src/widgets/filters_tool.cpp \
                    src/widgets/options_dialog.cpp \
                    src/widgets/resizable_grid.cpp \
                    src/widgets/flow_layout.cpp \
                    src/widgets/stream_card.cpp \
                    src/widgets/video_controls.cpp \
                    src/api/twitch.cpp \
                    src/api/twitchd.cpp \
                    src/api/oauth.cpp \

win32:SOURCES   +=  src/native/win32.cpp

HEADERS         +=  include/main_window.hpp \
                    include/constants.hpp \
                    include/libvlc.hpp \
                    include/vlc_logger.hpp \
                    include/vlc_event_watcher.hpp \
                    include/widgets/video_widget.hpp \
                    include/widgets/foreign_widget.hpp \
                    include/widgets/stream_widget.hpp \
                    include/widgets/stream_picker.hpp \
                    include/widgets/stream_container.hpp \
                    include/widgets/vlc_log_viewer.hpp \
                    include/widgets/filters_tool.hpp \
                    include/widgets/options_dialog.hpp \
                    include/widgets/resizable_grid.hpp \
                    include/widgets/flow_layout.hpp \
                    include/widgets/stream_card.hpp \
                    include/widgets/video_controls.hpp \
                    include/api/twitch.hpp \
                    include/api/twitchd.hpp \
                    include/api/oauth.hpp \
                    include/native/capabilities.hpp \
                    include/prelude/http.hpp \
                    include/prelude/sync.hpp \

win32:HEADERS   +=  include/native/win32.hpp

FORMS           +=  forms/main_window.ui \
                    forms/stream_picker.ui \
                    forms/vlc_log_viewer.ui \
                    forms/options_dialog.ui \
                    forms/stream_card.ui \
                    forms/filters_tool.ui \
                    forms/video_controls.ui \

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
