#pragma once

#include <QWidget>

#include <memory>

class QHBoxLayout;
class ForeignWidget;

class ChatPane: public QWidget {
    Q_OBJECT

public:
    ChatPane(QWidget * = nullptr);
    ~ChatPane();

private:
    QHBoxLayout *_layout;
    ForeignWidget *_chat;
};
