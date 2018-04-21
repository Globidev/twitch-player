#ifndef MAIN_WINDOW_HPP
#define MAIN_WINDOW_HPP

#include <set>
#include <QMainWindow>
#include "libvlc.hpp"

namespace Ui {
class MainWindow;
}

struct StreamWidget;

class MainWindow : public QMainWindow {
public:
    MainWindow(QWidget * = nullptr);
    MainWindow(QString, QWidget * = nullptr);
    ~MainWindow();

    void add_stream(QString);

protected:
    void moveEvent(QMoveEvent *) override;

private:
    Ui::MainWindow *ui;

    libvlc::Instance _video_context;
    std::set<StreamWidget *> streams;
};

#endif // MAIN_WINDOW_HPP
