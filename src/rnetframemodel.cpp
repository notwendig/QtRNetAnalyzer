#include "rnetframemodel.h"

#include <QRegularExpression>
#include <QStringList>
#include <QtGlobal>

#include <algorithm>

namespace {

QString textWithoutMetadata(QString text)
{
    const int brace = text.indexOf(QLatin1Char('{'));
    if (brace >= 0)
        text = text.left(brace);
    return text.trimmed();
}

QVector<QPair<QString, QString>> decodedPairsFromText(const RNetFrame &frame)
{
    QVector<QPair<QString, QString>> out;

    if (frame.name().compare(QStringLiteral("UNKNOWN"), Qt::CaseInsensitive) == 0)
        return out;

    const QString text = textWithoutMetadata(frame.toString());
    static const QRegularExpression rx(QStringLiteral(R"(([A-Za-z_][A-Za-z0-9_]*)\s*=\s*([^;,\)]+))"));

    auto it = rx.globalMatch(text);
    while (it.hasNext()) {
        const QRegularExpressionMatch m = it.next();
        QString key = m.captured(1).trimmed();
        QString value = m.captured(2).trimmed();
        if (key.isEmpty() || value.isEmpty())
            continue;
        out.push_back({key, value});
    }

    return out;
}

bool isIdPartKey(const QString &key)
{
    const QString k = key.toLower();
    return k == QStringLiteral("module")
        || k == QStringLiteral("device")
        || k == QStringLiteral("node")
        || k == QStringLiteral("address")
        || k == QStringLiteral("src")
        || k == QStringLiteral("dst");
}

QString joinPairs(const QVector<QPair<QString, QString>> &pairs, bool wantIdParts)
{
    QStringList parts;
    for (const auto &p : pairs) {
        const bool isId = isIdPartKey(p.first);
        if (isId != wantIdParts)
            continue;
        parts << QStringLiteral("%1=%2").arg(p.first, p.second);
    }
    return parts.join(QStringLiteral("; "));
}

} // namespace

RNetFrameModel::RNetFrameModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

int RNetFrameModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return static_cast<int>(m_rows.size());
}

int RNetFrameModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return ColumnCount;
}


QVariant RNetFrameModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return {};

    switch (section) {
    case ColPlot:
        return QStringLiteral("");
    case ColIndex:
        return QStringLiteral("#");
    case ColCount:
        return QStringLiteral("Count");
    case ColId:
        return QStringLiteral("ID");
    case ColName:
        return QStringLiteral("Name");
    case ColIdParts:
        return QStringLiteral("ID parts");
    case ColFields:
        return QStringLiteral("Fields");
    case ColData:
        return QStringLiteral("Data");
    case ColExt:
        return QStringLiteral("Ext");
    case ColRtr:
        return QStringLiteral("RTR");
    case ColTimestamp:
        return QStringLiteral("Timestamp");
    default:
        return {};
    }
}

QVariant RNetFrameModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return {};

    if (index.row() < 0 || index.row() >= static_cast<int>(m_rows.size()))
        return {};

    const RowBucket &bucket = m_rows[static_cast<std::size_t>(index.row())];

    if (bucket.history.empty())
        return {};

    const RNetFrame *frame = bucket.history.back().get();
    if (!frame)
        return {};

    if (role == Qt::CheckStateRole && index.column() == ColPlot)
        return m_taggedKeys.contains(bucket.key) ? Qt::Checked : Qt::Unchecked;

    if (role == Qt::ToolTipRole)
        return frame->toString();

    if (role == Qt::TextAlignmentRole) {
        switch (index.column()) {
        case ColPlot:
        case ColIndex:
        case ColCount:
        case ColExt:
        case ColRtr:
        case ColTimestamp:
            return QVariant::fromValue(int(Qt::AlignRight | Qt::AlignVCenter));
        default:
            return QVariant::fromValue(int(Qt::AlignLeft | Qt::AlignVCenter));
        }
    }

    if (role == Qt::UserRole) {
        switch (index.column()) {
        case ColPlot:
            return m_taggedKeys.contains(bucket.key) ? 1 : 0;
        case ColIndex:
            return index.row() + 1;
        case ColCount:
            return QVariant::fromValue<qulonglong>(static_cast<qulonglong>(bucket.totalCount));
        case ColId:
            return QVariant::fromValue<qulonglong>(static_cast<qulonglong>(frame->id));
        case ColName:
            return sortString(frame->name());
        case ColIdParts:
            return sortString(idPartsString(*frame));
        case ColFields:
            return sortString(fieldsString(*frame));
        case ColData:
            return sortString(formatPayload(frame->data));
        case ColExt:
            return frame->extended ? 1 : 0;
        case ColRtr:
            return frame->remote ? 1 : 0;
        case ColTimestamp:
            return QVariant::fromValue<double>(static_cast<double>(frame->hwTimestamp));
        default:
            return {};
        }
    }

    if (role != Qt::DisplayRole && role != Qt::EditRole)
        return {};

    switch (index.column()) {
    case ColPlot:
        return {};
    case ColIndex:
        return QString::number(index.row() + 1);
    case ColCount:
        return QString::number(static_cast<qulonglong>(bucket.totalCount));
    case ColId:
        return QStringLiteral("0x%1")
            .arg(frame->id, frame->extended ? 8 : 3, 16, QLatin1Char('0'))
            .toUpper();
    case ColName:
        return frame->name();
    case ColIdParts:
        return idPartsString(*frame);
    case ColFields:
        return fieldsString(*frame);
    case ColData:
        return formatPayload(frame->data);
    case ColExt:
        return frame->extended ? QStringLiteral("EXT") : QStringLiteral("STD");
    case ColRtr:
        return frame->remote ? QStringLiteral("RTR") : QString();
    case ColTimestamp:
        return QString::number(static_cast<double>(frame->hwTimestamp), 'f', 6);
    default:
        return {};
    }
}
Qt::ItemFlags RNetFrameModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags f = QAbstractTableModel::flags(index);
    if (index.isValid() && index.column() == ColPlot) {
        f |= Qt::ItemIsUserCheckable;
        f &= ~Qt::ItemIsEditable;
    }
    return f;
}

bool RNetFrameModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || index.column() != ColPlot || role != Qt::CheckStateRole)
        return false;
    if (index.row() < 0 || index.row() >= static_cast<int>(m_rows.size()))
        return false;

    const RowBucket &bucket = m_rows[static_cast<std::size_t>(index.row())];
    const bool enabled = value.toInt() == Qt::Checked;
    if (enabled)
        m_taggedKeys.insert(bucket.key);
    else
        m_taggedKeys.remove(bucket.key);

    const QString name = nameForKey(bucket.key);
    emit dataChanged(index, index, {Qt::CheckStateRole, Qt::DisplayRole, Qt::UserRole});
    emit tagStateChanged(bucket.key, name, enabled);
    return true;
}

void RNetFrameModel::clear()
{
    beginResetModel();
    m_rows.clear();
    m_rowByKey.clear();
    m_taggedKeys.clear();
    endResetModel();
}

void RNetFrameModel::addFrame(const CanFrame &frame)
{
    auto decoded = RNetFrame::decodeRNetMessage(frame);
    if (!decoded)
        return;

    const quint64 key = decoded->getKey();
    const QString name = decoded->name();

    auto it = m_rowByKey.find(key);
    if (it == m_rowByKey.end()) {
        const int row = static_cast<int>(m_rows.size());
        beginInsertRows(QModelIndex(), row, row);

        RowBucket bucket;
        bucket.key = key;
        bucket.totalCount = 1;
        bucket.updateThrottle.start();
        bucket.throttleStarted = true;
        bucket.history.push_back(std::move(decoded));
        m_rows.push_back(std::move(bucket));
        m_rowByKey.insert(key, row);

        endInsertRows();
        emit dataChanged(index(row, ColCount), index(row, ColTimestamp), {Qt::DisplayRole, Qt::UserRole, Qt::ToolTipRole});
    } else {
        const int row = it.value();
        if (row < 0 || row >= static_cast<int>(m_rows.size()))
            return;

        RowBucket &bucket = m_rows[static_cast<std::size_t>(row)];
        ++bucket.totalCount;
        bucket.history.push_back(std::move(decoded));

        if (bucket.history.size() > kMaxHistoryPerRow) {
            bucket.history.erase(bucket.history.begin(),
                                 bucket.history.begin() + (bucket.history.size() - kMaxHistoryPerRow));
        }

        if (!bucket.throttleStarted) {
            bucket.updateThrottle.start();
            bucket.throttleStarted = true;
        }

        emit dataChanged(index(row, ColCount), index(row, ColCount), {Qt::DisplayRole, Qt::UserRole});
        if (bucket.updateThrottle.elapsed() >= kUiUpdateIntervalMs) {
            emit dataChanged(index(row, ColName), index(row, ColTimestamp), {Qt::DisplayRole, Qt::UserRole, Qt::ToolTipRole});
            bucket.updateThrottle.restart();
        }
    }

    if (m_taggedKeys.contains(key))
        emit taggedFrameReceived(key, name, frame);
}

const RNetFrame *RNetFrameModel::latestFrameAt(int row) const
{
    if (row < 0 || row >= static_cast<int>(m_rows.size()))
        return nullptr;
    const RowBucket &bucket = m_rows[static_cast<std::size_t>(row)];
    if (bucket.history.empty())
        return nullptr;
    return bucket.history.back().get();
}

const std::vector<std::unique_ptr<RNetFrame>> *RNetFrameModel::historyAt(int row) const
{
    if (row < 0 || row >= static_cast<int>(m_rows.size()))
        return nullptr;
    return &m_rows[static_cast<std::size_t>(row)].history;
}

const std::vector<std::unique_ptr<RNetFrame>> *RNetFrameModel::historyForKey(quint64 key) const
{
    auto it = m_rowByKey.constFind(key);
    if (it == m_rowByKey.constEnd())
        return nullptr;
    return historyAt(it.value());
}

QString RNetFrameModel::nameForKey(quint64 key) const
{
    const auto *history = historyForKey(key);
    if (!history || history->empty() || !history->back())
        return QString();
    return history->back()->name();
}

QString RNetFrameModel::formatPayload(const QByteArray &data)
{
    if (data.isEmpty())
        return QStringLiteral("-");

    QString out;
    for (int i = 0; i < data.size(); ++i) {
        if (i)
            out += QLatin1Char(' ');
        out += QStringLiteral("%1")
            .arg(static_cast<quint8>(data.at(i)), 2, 16, QLatin1Char('0'))
            .toUpper();
    }
    return out;
}

QString RNetFrameModel::idPartsString(const RNetFrame &frame)
{
    return joinPairs(decodedPairsFromText(frame), true);
}

QString RNetFrameModel::fieldsString(const RNetFrame &frame)
{
    return joinPairs(decodedPairsFromText(frame), false);
}

QString RNetFrameModel::sortString(QString value)
{
    return value.toCaseFolded();
}
