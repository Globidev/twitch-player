#ifndef MAIN_WINDOW_HPP
#define MAIN_WINDOW_HPP

#include "libvlc.hpp"

#include <QMainWindow>

#include <vector>
#include <set>

namespace Ui {
class MainWindow;
}

class QSplitter;
class StreamContainer;
class VLCLogViewer;

class MainWindow : public QMainWindow {
public:
    MainWindow(QWidget * = nullptr);
    ~MainWindow();

    void add_picker(int, int);
    void add_stream(int, int, QString);

protected:
    void changeEvent(QEvent *) override;
    void keyPressEvent(QKeyEvent *) override;
    void mousePressEvent(QMouseEvent *) override;

private:
    libvlc::Instance _video_context;

    Ui::MainWindow *_ui;
    std::vector<QSplitter *> rows;
    std::set<StreamContainer *> _streams;
    QSplitter *_main_splitter;
    VLCLogViewer *_vlc_log_viewer;

    void toggle_fullscreen();
    std::pair<int, int> find_focused_stream();
};

#endif // MAIN_WINDOW_HPP
