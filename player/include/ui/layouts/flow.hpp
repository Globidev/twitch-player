#pragma once

// Adapted from
// http://doc.qt.io/qt-5/qtwidgets-layouts-flowlayout-flowlayout-h.html

#include <QLayout>
#include <QRect>
#include <QStyle>

class FlowLayout: public QLayout {
public:
    explicit FlowLayout(QWidget * = nullptr);
    ~FlowLayout();

    void addItem(QLayoutItem *) override;
    int horizontalSpacing() const;
    int verticalSpacing() const;
    Qt::Orientations expandingDirections() const override;
    bool hasHeightForWidth() const override;
    int heightForWidth(int) const override;
    int count() const override;
    QLayoutItem *itemAt(int) const override;
    QSize minimumSize() const override;
    void setGeometry(const QRect &) override;
    QSize sizeHint() const override;
    QLayoutItem *takeAt(int) override;

private:
    int doLayout(const QRect &, bool) const;
    int smartSpacing(QStyle::PixelMetric) const;

    QList<QLayoutItem *> itemList;
};
