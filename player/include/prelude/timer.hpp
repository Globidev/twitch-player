#pragma once

#include <QTimer>

template <class Slot>
void interval(QObject *parent, int ms, Slot && slot) {
    auto timer = new QTimer(parent);
    QObject::connect(timer, &QTimer::timeout, std::forward<Slot>(slot));
    timer->start(ms);
}

template <class Slot>
void delayed(QObject *parent, int ms, Slot && slot) {
    auto timer = new QTimer(parent);
    QObject::connect(timer, &QTimer::timeout, std::forward<Slot>(slot));
    timer->setSingleShot(true);
    timer->start(ms);
}
