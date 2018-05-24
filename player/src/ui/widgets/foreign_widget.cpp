#include "ui/widgets/foreign_widget.hpp"

#include "ui/native/capabilities.hpp"

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

    auto abstract_handle = from_native_handle(handle);
    if (auto win_ptr = QWindow::fromWinId(abstract_handle); win_ptr) {
        _foreign_win_ptr = win_ptr;
        if (auto container = QWidget::createWindowContainer(win_ptr); container) {
            _container = container;
            _layout->addWidget(container);
            // Forcing an initial redraw seems to help fixing reparenting issues
            redraw();
        }
    }
}

ForeignWidget::~ForeignWidget() {
    release_window();
}

void ForeignWidget::release_window() {
    if (_foreign_win_ptr) {
        auto win_ptr = *_foreign_win_ptr;
        auto handle = to_native_handle(win_ptr->winId());

        win_ptr->setParent(nullptr);
        sysclose_window(handle);
    }
}

void ForeignWidget::redraw()  {
    // On Windows, there are sometimes issues where the window won't redraw
    // itself when the container or its parents are reparented
    if (_container)
        ::redraw(to_native_handle((*_container)->winId()));
}
