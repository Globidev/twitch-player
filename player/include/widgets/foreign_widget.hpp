#ifndef FOREIGN_WIDGET_HPP
#define FOREIGN_WIDGET_HPP

#include <QWidget>

#include <optional>

#include "native/capabilities.hpp"

class QHBoxLayout;

class ForeignWidget: public QWidget {
public:
    ForeignWidget(QWidget * = nullptr);
    ~ForeignWidget();

    void grab(WindowHandle);

protected:
    void showEvent(QShowEvent *) override;

private:
    std::optional<QWindow *> _foreign_win_ptr;
    std::optional<QWidget *> _container;
    QHBoxLayout *_layout;

    void release_window();
};

#endif // FOREIGN_WIDGET_HPP
