#pragma once

#include <QObject>
#include <QEvent>

#include <unordered_set>

class EventNotifier: public QObject {
    Q_OBJECT

public:
    EventNotifier(std::initializer_list<QEvent::Type>, QObject * = nullptr);

signals:
    void new_event(const QEvent &);

protected:
    bool eventFilter(QObject *, QEvent *) override;

private:
    std::unordered_set<QEvent::Type> _monitored_events;
};
