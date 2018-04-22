#ifndef MAIN_WINDOW_HPP
#define MAIN_WINDOW_HPP

#include "libvlc.hpp"

#include <QMainWindow>

#include <vector>

namespace Ui {
class MainWindow;
}

class QSplitter;
class StreamWidget;

class MainWindow : public QMainWindow {
public:
    MainWindow(QWidget * = nullptr);
    MainWindow(QString, QWidget * = nullptr);

    void add_stream(QString, int, int);

protected:
    void changeEvent(QEvent *) override;
    void keyPressEvent(QKeyEvent *) override;

private:
    libvlc::Instance _video_context;

    Ui::MainWindow *_ui;
    std::vector<QSplitter *> rows;
    std::vector<StreamWidget *> _streams;
    QSplitter *_main_splitter;
    QShortcut *_sh_full_screen;

    void add_stream_impl(QString, HWND, int, int);
    void toggle_fullscreen();
};

#endif // MAIN_WINDOW_HPP
