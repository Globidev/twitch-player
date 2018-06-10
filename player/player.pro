TARGET          =   twitch-player
TEMPLATE        =   app

QT              +=  core gui widgets network
QTPLUGIN        +=  qsvg

include(vendor/qtpromise-0.3.0/qtpromise.pri)

win32:QMAKE_CXXFLAGS    +=  /std:c++latest

unix {
    # Only way I found to make it compile with c++17
    CONFIG -= c++11
    QMAKE_CXXFLAGS += -std=c++17
}

release:CONFIG  +=  static

SOURCES         +=  src/main.cpp \
                    \
                    src/api/twitch.cpp \
                    src/api/twitchd.cpp \
                    src/api/oauth.cpp \
                    \
                    src/libvlc/bindings.cpp \
                    src/libvlc/event_watcher.cpp \
                    src/libvlc/logger.cpp \
                    \
                    src/process/daemon_control.cpp \
                    \
                    src/ui/main_window.cpp \
                    src/ui/main_window_shortcuts.cpp \
                    \
                    src/ui/layouts/flow.cpp \
                    src/ui/layouts/splitter_grid.cpp \
                    \
                    src/ui/overlays/video_controls.cpp \
                    src/ui/overlays/video_details.cpp \
                    \
                    src/ui/tools/options_dialog.cpp \
                    src/ui/tools/video_filters.cpp \
                    src/ui/tools/vlc_log_viewer.cpp \
                    \
                    src/ui/utils/event_notifier.cpp \
                    \
                    src/ui/widgets/foreign_widget.cpp \
                    src/ui/widgets/pane.cpp \
                    src/ui/widgets/stream_card.cpp \
                    src/ui/widgets/stream_picker.cpp \
                    src/ui/widgets/stream_widget.cpp \
                    src/ui/widgets/video_widget.cpp \

HEADERS         +=  include/constants.hpp \
                    \
                    include/api/twitch.hpp \
                    include/api/twitchd.hpp \
                    include/api/oauth.hpp \
                    \
                    include/libvlc/bindings.hpp \
                    include/libvlc/event_watcher.hpp \
                    include/libvlc/logger.hpp \
                    include/libvlc/types.hpp \
                    \
                    include/prelude/c_wrapper.hpp \
                    include/prelude/http.hpp \
                    include/prelude/promise.hpp \
                    include/prelude/sync.hpp \
                    include/prelude/timer.hpp \
                    include/prelude/variant.hpp \
                    \
                    include/process/daemon_control.hpp \
                    \
                    include/ui/main_window.hpp \
                    \
                    include/ui/layouts/flow.hpp \
                    include/ui/layouts/splitter_grid.hpp \
                    \
                    include/ui/native/capabilities.hpp \
                    \
                    include/ui/overlays/video_controls.hpp \
                    include/ui/overlays/video_details.hpp \
                    \
                    include/ui/tools/options_dialog.hpp \
                    include/ui/tools/video_filters.hpp \
                    include/ui/tools/vlc_log_viewer.hpp \
                    \
                    include/ui/utils/event_notifier.hpp \
                    \
                    include/ui/widgets/foreign_widget.hpp \
                    include/ui/widgets/pane.hpp \
                    include/ui/widgets/stream_card.hpp \
                    include/ui/widgets/stream_picker.hpp \
                    include/ui/widgets/stream_widget.hpp \
                    include/ui/widgets/video_widget.hpp \

win32:SOURCES   +=  src/ui/native/win32.cpp
win32:HEADERS   +=  include/ui/native/win32.hpp

macx:SOURCES    +=  src/ui/native/osx.cpp
macx:HEADERS    +=  include/ui/native/osx.hpp

unix:!macx:SOURCES    +=  src/ui/native/x11.cpp
unix:!macx:HEADERS    +=  include/ui/native/x11.hpp

FORMS           +=  forms/main_window.ui \
                    forms/stream_picker.ui \
                    forms/vlc_log_viewer.ui \
                    forms/options_dialog.ui \
                    forms/stream_card.ui \
                    forms/filters_tool.ui \
                    forms/video_controls.ui \
                    forms/stream_details.ui \

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

LIBS            +=  -L$${LIBVLC_LIB_DIR}
unix:LIBS       +=  -lvlc
unix:!macx:LIBS +=  -lX11
win32:LIBS      +=  -llibvlc -luser32

release:DESTDIR = release
debug:DESTDIR   = debug

MOC_DIR         = $${DESTDIR}/.moc
OBJECTS_DIR     = $${DESTDIR}/.obj
RCC_DIR         = $${DESTDIR}/.rcc
UI_DIR          = $${DESTDIR}/.ui

win32:RC_ICONS  += resources/icon.ico
