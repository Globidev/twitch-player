#include "stream_widget.hpp"

StreamWidget::StreamWidget(libvlc::Instance &inst, void *hwnd, QWidget *parent):
    QSplitter(parent),
    video(inst),
    chat(hwnd)
{
    addWidget(&video);
    addWidget(&chat);
    setSizes(QList<int>() << 80 << 20);
}
