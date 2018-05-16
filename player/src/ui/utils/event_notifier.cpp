#include "ui/utils/event_notifier.hpp"

EventNotifier::EventNotifier(std::initializer_list<QEvent::Type> events, QObject *parent):
    QObject(parent),
    _monitored_events(events)
{ }

bool EventNotifier::eventFilter(QObject *watched, QEvent *event) {
    if (_monitored_events.count(event->type()))
        emit new_event(*event);

    return QObject::eventFilter(watched, event);
}

