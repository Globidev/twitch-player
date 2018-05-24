#pragma once

#include <QWidget>
#include <vector>

class QSplitter;
class QHBoxLayout;

struct Position {
    int row, column;

    Position left(Qt::Orientation orientation = Qt::Vertical) {
        return orientation == Qt::Vertical
             ? Position { row, column - 1 }
             : up();
    }
    Position right(Qt::Orientation orientation = Qt::Vertical) {
        return orientation == Qt::Vertical
             ? Position { row, column + 1 }
             : down();
    }
    Position up(Qt::Orientation orientation = Qt::Vertical) {
        return orientation == Qt::Vertical
             ? Position { row - 1, column }
             : left();
    }
    Position down(Qt::Orientation orientation = Qt::Vertical) {
        return orientation == Qt::Vertical
             ? Position { row + 1, column }
             : right();
    }
};

class SplitterGrid: public QWidget {
public:
    SplitterGrid(QWidget * = nullptr);

    void insert_widget(Position, QWidget *);
    void remove_widget(QWidget *);
    Position widget_position(QWidget *);
    QWidget * closest_widget(Position);
    void swap(Position, Position);
    void rotate();
    Qt::Orientation orientation() const;

private:
    Qt::Orientation _main_orientation = Qt::Vertical;

    QHBoxLayout *_layout;
    QSplitter *_main_splitter;
    std::vector<QSplitter *> _inner_splitters;

    QSplitter * get_inner(int);
};
