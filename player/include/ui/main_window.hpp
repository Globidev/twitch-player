#pragma once

#include <QMainWindow>
#include <QMap>

#include <vector>
#include <tuple>
#include <variant>
#include <optional>
#include <memory>

#include "ui/layouts/splitter_grid.hpp"

namespace Ui {
class MainWindow;
}

namespace libvlc {
struct Instance;
}

class StreamPane;
class ChatPane;
class VLCLogViewer;
class QStackedWidget;
class QShortcut;

class TwitchPubSub;

using MPane = std::variant<
    StreamPane *,
    ChatPane *
>;

class MainWindow : public QMainWindow {
public:
    MainWindow(libvlc::Instance &, TwitchPubSub &, QWidget * = nullptr);
    ~MainWindow();

    StreamPane *add_stream_pane(Position);
    ChatPane *add_chat_pane(Position);
    void remove_pane(MPane);

protected:
    void changeEvent(QEvent *) override;
    void keyPressEvent(QKeyEvent *) override;

private:
    std::unique_ptr<Ui::MainWindow> _ui;

    libvlc::Instance &_video_context;
    TwitchPubSub &_pubsub;

    std::unique_ptr<VLCLogViewer> _vlc_log_viewer;

    std::vector<MPane> _panes;
    SplitterGrid *_grid;
    QStackedWidget *_central_widget;

    Position _zoomed_position;

    std::optional<MPane> focused_pane();

    void move_focus(Position);
    void move_pane(Position, Position);

    void set_fullscreen(bool);
    bool is_zoomed();
    void zoom(StreamPane *);
    void unzoom();

    std::vector<std::pair<QString, std::pair<QShortcut *, QAction*>>> _shortcuts;

    void setup_shortcuts();
    void remap_shortcuts();

    QMenu *_audio_devices_menu;
    void setup_audio_devices();

    QAction *_action_stream_zoom;
    QAction *_action_fullscreen;
};
