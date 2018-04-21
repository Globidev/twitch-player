#include "foreign_widget.hpp"

#include <QApplication>
#include <QDebug>

#include <windows.h>

ForeignWidget::ForeignWidget(void *handle, QWidget *parent):
    QWidget(parent), _handle(handle)
{
    _win = QWindow::fromWinId(reinterpret_cast<WId>(handle));
    if (_win) {
        _win_geometry = _win->geometry();
        _container = QWidget::createWindowContainer(_win);
        if (_container)
            _layout.addWidget(_container);
    }

    _layout.setContentsMargins(0, 0, 0, 0);
    setContentsMargins(0, 0, 0, 0);
    setLayout(&_layout);
}

ForeignWidget::~ForeignWidget() {
    if (_win) {
        _win->setGeometry(_win_geometry);
        _win->setParent(nullptr);
        SendMessage(reinterpret_cast<HWND>(_handle), WM_SYSCOMMAND, SC_CLOSE, 0);
    }
}
