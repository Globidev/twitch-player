#ifndef RESIZABLE_GRID_HPP
#define RESIZABLE_GRID_HPP

#include <QWidget>
#include <vector>

class QSplitter;
class QHBoxLayout;

struct Position {
    int row, column;

    auto left()  { return Position { row, column - 1 }; }
    auto right() { return Position { row, column + 1 }; }
    auto up()    { return Position { row - 1, column }; }
    auto down()  { return Position { row + 1, column }; }
};

class ResizableGrid: public QWidget {
public:
    ResizableGrid(QWidget * = nullptr);

    void insert_widget(Position, QWidget *);
    void remove_widget(QWidget *);
    Position widget_position(QWidget *);
    QWidget * closest_widget(Position);

private:
    enum class MainDirection { Vertical, Horizontal };

    QHBoxLayout *_layout;
    QSplitter *_main_splitter;
    std::vector<QSplitter *> _inner_splitters;

    MainDirection _main_direction = MainDirection::Vertical;

    QSplitter * get_inner(int);
};

#endif // RESIZABLE_GRID_HPP
