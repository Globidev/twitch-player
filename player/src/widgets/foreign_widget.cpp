#include "widgets/foreign_widget.hpp"

#include <QApplication>
#include <QDebug>

#include <windows.h>

ForeignWidget::ForeignWidget(void *handle, QWidget *parent):
    QWidget(parent), _handle(handle)
{
    _layout.setContentsMargins(QMargins());
    setContentsMargins(QMargins());

    if (_win = QWindow::fromWinId(reinterpret_cast<WId>(handle)); _win) {
        if (auto container = QWidget::createWindowContainer(_win); container)
            _layout.addWidget(container);
    }

    setLayout(&_layout);
}

ForeignWidget::~ForeignWidget() {
    if (_win) {
        _win->setParent(nullptr);
        SendMessage(reinterpret_cast<HWND>(_handle), WM_SYSCOMMAND, SC_CLOSE, 0);
    }
}
