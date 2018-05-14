#pragma once

#include <QWidget>

#include <optional>

#include "ui/native/capabilities.hpp"

class QHBoxLayout;

class ForeignWidget: public QWidget {
public:
    ForeignWidget(QWidget * = nullptr);
    ~ForeignWidget();

    void grab(WindowHandle);
    void redraw();

private:
    std::optional<QWindow *> _foreign_win_ptr;
    std::optional<QWidget *> _container;
    QHBoxLayout *_layout;

    void release_window();
};
