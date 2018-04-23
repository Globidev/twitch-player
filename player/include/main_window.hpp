#ifndef MAIN_WINDOW_HPP
#define MAIN_WINDOW_HPP

#include "libvlc.hpp"

#include <QMainWindow>

#include <vector>

namespace Ui {
class MainWindow;
}

class QSplitter;
class StreamContainer;

class MainWindow : public QMainWindow {
public:
    MainWindow(QWidget * = nullptr);

    void add_picker(int, int);
    void add_stream(int, int, QString);

protected:
    void changeEvent(QEvent *) override;
    void keyPressEvent(QKeyEvent *) override;

private:
    libvlc::Instance _video_context;

    Ui::MainWindow *_ui;
    std::vector<QSplitter *> rows;
    std::vector<StreamContainer *> _streams;
    QSplitter *_main_splitter;
    QShortcut *_sh_full_screen;

    void toggle_fullscreen();
};

#endif // MAIN_WINDOW_HPP
