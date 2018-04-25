#include "widgets/foreign_widget.hpp"

#include <QHBoxLayout>
#include <QWindow>

ForeignWidget::ForeignWidget(QWidget *parent):
    QWidget(parent),
    _layout(new QHBoxLayout(this))
{
    setLayout(_layout);

    _layout->setContentsMargins(QMargins());
    setContentsMargins(QMargins());
}

void ForeignWidget::grab(WindowHandle handle) {
    release_window();

    auto abstract_handle = reinterpret_cast<WId>(handle);
    if (auto win_ptr = QWindow::fromWinId(abstract_handle); win_ptr) {
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
        auto win_ptr = *_foreign_win_ptr;
        auto handle = reinterpret_cast<WindowHandle>(win_ptr->winId());

        win_ptr->setParent(nullptr);
        sysclose_window(handle);
    }
}
