#include "widgets/foreign_widget.hpp"

#include <QHBoxLayout>
#include <QWindow>

#include <windows.h>

ForeignWidget::ForeignWidget(QWidget *parent):
    QWidget(parent),
    _layout(new QHBoxLayout(this))
{
    setLayout(_layout);

    _layout->setContentsMargins(QMargins());
    setContentsMargins(QMargins());
}

void ForeignWidget::grab(void *handle) {
    release_window();

    auto native_handle = reinterpret_cast<WId>(handle);
    if (auto win_ptr = QWindow::fromWinId(native_handle); win_ptr) {
        _foreign_win_ptr = win_ptr;
        if (auto container = QWidget::createWindowContainer(win_ptr); container)
            _layout->addWidget(container);
    }
}

ForeignWidget::~ForeignWidget() {
    release_window();
}

void ForeignWidget::release_window() {
    if (_foreign_win_ptr) {
        (*_foreign_win_ptr)->setParent(nullptr);
        auto handle = reinterpret_cast<HWND>((*_foreign_win_ptr)->winId());
        SendMessage(handle, WM_SYSCOMMAND, SC_CLOSE, 0);
    }
}
