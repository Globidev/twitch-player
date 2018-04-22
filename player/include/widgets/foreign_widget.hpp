#ifndef FOREIGN_WIDGET_HPP
#define FOREIGN_WIDGET_HPP

#include <QWidget>
#include <QWindow>
#include <QHBoxLayout>

class ForeignWidget: public QWidget {
public:
    ForeignWidget(void *handle, QWidget * = nullptr);
    ~ForeignWidget();

private:
    QWindow *_win;
    QHBoxLayout _layout;
    void * _handle;
};

#endif // FOREIGN_WIDGET_HPP
