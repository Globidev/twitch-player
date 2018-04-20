#ifndef MAIN_WINDOW_HPP
#define MAIN_WINDOW_HPP

#include <QMainWindow>
#include "libvlc.hpp"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {
public:
    MainWindow(QWidget * = nullptr);
    MainWindow(QString, QWidget * = nullptr);
    ~MainWindow();

    void add_stream(QString);

private:
    Ui::MainWindow *ui;

    libvlc::Instance _video_context;
};

#endif // MAIN_WINDOW_HPP
