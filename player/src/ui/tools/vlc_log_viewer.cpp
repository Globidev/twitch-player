#include "ui/tools/vlc_log_viewer.hpp"
#include "ui_vlc_log_viewer.h"

#include "libvlc/logger.hpp"

static QColor level_color(libvlc::LogLevel log_level) {
    switch (log_level) {
        case libvlc::LogLevel::Debug:   return Qt::gray;
        case libvlc::LogLevel::Notice:  return Qt::green;
        case libvlc::LogLevel::Warning: return QColor(0xFF, 0xA5, 0x00);
        case libvlc::LogLevel::Error:   return Qt::red;
        case libvlc::LogLevel::Unknown:
        default:                        return Qt::black;
    }
}

static QString level_string(libvlc::LogLevel log_level) {
    switch (log_level) {
        case libvlc::LogLevel::Debug:   return "Debug";
        case libvlc::LogLevel::Notice:  return "Notice";
        case libvlc::LogLevel::Warning: return "Warning";
        case libvlc::LogLevel::Error:   return "Error";
        case libvlc::LogLevel::Unknown:
        default:                        return "Unknown";
    }
}

VLCLogViewer::VLCLogViewer(libvlc::Instance &video_context, QWidget *parent):
    QWidget(parent),
    _ui(std::make_unique<Ui::VLCLogViewer>()),
    _item_model(new VLCLogItemModel(this)),
    _filter_proxy_model(new VLCLogItemFilter(*_item_model, this)),
    _logger(new VLCLogger(video_context, this))
{
    _ui->setupUi(this);

    _ui->tableView->setModel(_filter_proxy_model);
    _ui->tableView->horizontalHeader()
        ->setSectionResizeMode(QHeaderView::ResizeToContents);

    QObject::connect(
        _logger,
        &VLCLogger::newLogEntry,
        [this](auto entry) {
            _item_model->add_log_entry(entry);
            auto as_text = QString("[%1] %2")
                .arg(level_string(entry.level), QString::fromStdString(entry.text));
            _ui->textView->append(as_text);
        }
    );

    auto add_filter = [this](auto check_box, auto log_level) {
        QObject::connect(check_box, &QCheckBox::toggled, [=](bool toggled) {
            _filter_proxy_model->toggle_level(log_level, toggled);
        });
    };

    add_filter(_ui->checkDebug,   libvlc::LogLevel::Debug);
    add_filter(_ui->checkNotice,  libvlc::LogLevel::Notice);
    add_filter(_ui->checkWarning, libvlc::LogLevel::Warning);
    add_filter(_ui->checkError,   libvlc::LogLevel::Error);
}

VLCLogItemModel::VLCLogItemModel(QObject *parent):
    QAbstractItemModel(parent)
{ }

void VLCLogItemModel::add_log_entry(libvlc::LogEntry entry) {
    auto index = static_cast<int>(entries.size());
    beginInsertRows(QModelIndex(), index, index);
    entries.push_back(LogEntry { QDateTime::currentDateTime(), entry });
    endInsertRows();
}

QModelIndex VLCLogItemModel::index(int row, int col, const QModelIndex &) const {
    return createIndex(row, col);
}

QModelIndex VLCLogItemModel::parent(const QModelIndex &) const {
    return QModelIndex();
}

int VLCLogItemModel::rowCount(const QModelIndex &) const {
    return static_cast<int>(entries.size());
}

int VLCLogItemModel::columnCount(const QModelIndex &) const {
    return 3;
}

QVariant VLCLogItemModel::data(const QModelIndex &index, int role) const {
    if (index.row() >= entries.size())
        return QVariant();

    auto & entry = entries[index.row()];

    switch (index.column()) {
        case 0:
            switch (role) {
                case Qt::DecorationRole:
                    return level_color(entry.data.level);
                case Qt::ToolTipRole:
                    return level_string(entry.data.level);
                default:
                    return QVariant();
            }
        case 1:
            switch (role) {
                case Qt::DisplayRole:
                    return entry.stamp.time().toString();
                case Qt::ToolTipRole:
                    return entry.stamp.toString("yyyy-MM-dd hh:mm:ss.zzz");
                default:
                    return QVariant();
            }
        case 2:
            switch (role) {
                case Qt::DisplayRole:
                    return QString::fromStdString(entry.data.text);
                default:
                    return QVariant();
            }
        default:
            return QVariant();
    }
}

VLCLogItemFilter::VLCLogItemFilter(VLCLogItemModel &model, QObject *parent):
    QSortFilterProxyModel(parent),
    _model(model)
{
    setSourceModel(&model);
}

bool VLCLogItemFilter::filterAcceptsRow(int row, const QModelIndex &) const {
    if (row >= _model.entries.size())
        return false;

    return _filtered_levels.count(_model.entries[row].data.level) == 0;
}

void VLCLogItemFilter::toggle_level(libvlc::LogLevel log_level, bool toggled) {
    if (toggled) _filtered_levels.erase(log_level);
    else         _filtered_levels.insert(log_level);
    invalidateFilter();
}
