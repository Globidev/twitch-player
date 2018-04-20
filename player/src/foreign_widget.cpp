#include "foreign_widget.hpp"

#include <QApplication>

ForeignWidget::ForeignWidget(void *handle, QWidget *parent):
    QWidget(parent)
{
    _layout = new QHBoxLayout(this);
    _win = QWindow::fromWinId(reinterpret_cast<WId>(handle));
    if (_win) {
        _win_geometry = _win->geometry();
        _container = QWidget::createWindowContainer(_win);
        if (_container)
            _layout->addWidget(_container);
    }

    _layout->setContentsMargins(0, 0, 0, 0);
    setContentsMargins(0, 0, 0, 0);
    setLayout(_layout);
}

ForeignWidget::~ForeignWidget() {
    if (_win) {
        _win->setGeometry(_win_geometry);
        _win->setParent(nullptr);
    }
}
