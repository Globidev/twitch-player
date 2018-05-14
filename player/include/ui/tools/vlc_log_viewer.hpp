#pragma once

#include <QWidget>
#include <QDateTime>
#include <QAbstractItemModel>
#include <QSortFilterProxyModel>

#include <vector>
#include <set>
#include <mutex>

#include "libvlc/bindings.hpp"

namespace Ui {
    class VLCLogViewer;
}

class VLCLogger;

class VLCLogItemModel: public QAbstractItemModel {
public:
    VLCLogItemModel(QObject * = nullptr);

    QModelIndex index(int, int, const QModelIndex & = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &) const override;
    int rowCount(const QModelIndex & = QModelIndex()) const override;
    int columnCount(const QModelIndex & = QModelIndex()) const override;
    QVariant data(const QModelIndex &, int = Qt::DisplayRole) const override;

    void add_log_entry(libvlc::LogEntry);

    struct LogEntry {
        QDateTime stamp;
        libvlc::LogEntry data;
    };
    std::vector<LogEntry> entries;
};

class VLCLogItemFilter: public QSortFilterProxyModel {
public:
    VLCLogItemFilter(VLCLogItemModel &, QObject * = nullptr);

    bool filterAcceptsRow(int, const QModelIndex &) const override;

    void toggle_level(libvlc::LogLevel, bool);
private:
    std::set<libvlc::LogLevel> _filtered_levels;
    VLCLogItemModel &_model;
};

class VLCLogViewer: public QWidget {
public:
    VLCLogViewer(libvlc::Instance &, QWidget * = nullptr);
    ~VLCLogViewer();

private:
    Ui::VLCLogViewer *_ui;
    VLCLogItemModel *_item_model;
    VLCLogItemFilter *_filter_proxy_model;
    VLCLogger *_logger;
    std::mutex _log_mutex;
};
