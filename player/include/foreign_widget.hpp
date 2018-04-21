#ifndef FOREIGN_WIDGET_HPP
#define FOREIGN_WIDGET_HPP

#include <QWidget>
#include <QWindow>
#include <QHBoxLayout>

struct ForeignWidget: public QWidget {
    ForeignWidget(void *handle, QWidget * = nullptr);
    ~ForeignWidget();

  private:
    QWindow *_win;
    QWidget *_container;
    QHBoxLayout _layout;
    QRect _win_geometry;
    void * _handle;
};

#endif // FOREIGN_WIDGET_HPP
