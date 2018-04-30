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

void ResizableGrid::insert_widget(Position pos, QWidget *widget) {
    auto inner_splitter = get_inner(pos.row);
    inner_splitter->insertWidget(pos.column, widget);
    inner_splitter->setSizes(equal_sizes(inner_splitter->count()));
}

void ResizableGrid::remove_widget(QWidget *widget) {
    auto pos = widget_position(widget);

    if (pos.row < _inner_splitters.size()) {
        auto inner_splitter_it = _inner_splitters.begin();
        std::advance(inner_splitter_it, pos.row);
        if ((*inner_splitter_it)->count() == 1) {
            (*inner_splitter_it)->deleteLater();
            _inner_splitters.erase(inner_splitter_it);
        }
    }
}

Position ResizableGrid::widget_position(QWidget *widget) {
    for (int row = 0; row < _inner_splitters.size(); ++row)
        for (int col = 0; col < _inner_splitters[row]->count(); ++col)
            if (_inner_splitters[row]->widget(col) == widget)
                return { row, col };

    return { 0, 0 };
}

QWidget * ResizableGrid::closest_widget(Position pos) {
    if (_inner_splitters.empty())
        return nullptr;

    auto max_row = static_cast<int>(_inner_splitters.size() - 1);
    auto row = std::max(std::min(pos.row, max_row), 0);
    auto inner_splitter = _inner_splitters[row];
    auto col = std::max(std::min(pos.column, inner_splitter->count() - 1), 0);

    return inner_splitter->widget(col);
}

void ResizableGrid::swap(Position from, Position to) {
    auto from_w = closest_widget(from);
    auto to_w = closest_widget(to);
    if (from_w && from_w != to_w) {
        auto actual_from = widget_position(from_w);
        auto actual_to = widget_position(to_w);
        auto splitter_from = _inner_splitters[actual_from.row];
        auto splitter_to = _inner_splitters[actual_to.row];
        auto splitter_from_sizes = splitter_from->sizes();
        auto splitter_to_sizes = splitter_to->sizes();
        splitter_from->replaceWidget(actual_from.column, to_w);
        splitter_to->insertWidget(actual_to.column, from_w);
        splitter_from->setSizes(splitter_from_sizes);
        splitter_to->setSizes(splitter_to_sizes);
    }
}

QSplitter * ResizableGrid::get_inner(int row) {
    if (row < 0) {
        auto splitter = new QSplitter(this);
        _main_splitter->insertWidget(0, splitter);
        _main_splitter->setSizes(equal_sizes(_main_splitter->count()));
        _inner_splitters.insert(_inner_splitters.begin(), splitter);
        return splitter;
    }
    else if (row >= _inner_splitters.size()) {
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
