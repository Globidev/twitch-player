#include "widgets/resizable_grid.hpp"

#include <QHBoxLayout>
#include <QSplitter>

static auto equal_sizes(int size) {
    QList<int> sizes;
    for (int i = 0; i < size; ++i)
        sizes << 100;
    return sizes;
}

ResizableGrid::ResizableGrid(QWidget *parent):
    QWidget(parent),
    _layout(new QHBoxLayout(this)),
    _main_splitter(new QSplitter(Qt::Vertical, this))
{
    _layout->setContentsMargins(QMargins());
    _layout->addWidget(_main_splitter);

    setLayout(_layout);
}

void ResizableGrid::insert_widget(int row, int col, QWidget *widget) {
    auto inner_splitter = get_inner(row);
    inner_splitter->insertWidget(col, widget);
    inner_splitter->setSizes(equal_sizes(inner_splitter->count()));
}

QSplitter * ResizableGrid::get_inner(int row) {
    if (row >= _inner_splitters.size()) {
        auto splitter = new QSplitter(this);
        _main_splitter->addWidget(splitter);
        _main_splitter->setSizes(equal_sizes(_main_splitter->count()));
        _inner_splitters.push_back(splitter);
        return splitter;
    }
    else {
        return _inner_splitters[row];
    }
}
