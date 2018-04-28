#ifndef RESIZABLE_GRID_HPP
#define RESIZABLE_GRID_HPP

#include <QWidget>
#include <vector>

class QSplitter;
class QHBoxLayout;

class ResizableGrid: public QWidget {
public:
    ResizableGrid(QWidget * = nullptr);

    void insert_widget(int, int, QWidget *);

private:
    enum class MainDirection { Vertical, Horizontal };

    QHBoxLayout *_layout;
    QSplitter *_main_splitter;
    std::vector<QSplitter *> _inner_splitters;

    MainDirection _main_direction = MainDirection::Vertical;

    QSplitter * get_inner(int);
};

#endif // RESIZABLE_GRID_HPP
