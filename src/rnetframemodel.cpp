// SPDX-License-Identifier: GPL-3.0-only
/*
 * QtRNetAnalyzer
 *
 * Copyright (c) 2026
 * ChatGPT (GPT-5.4 Thinking)
 * Jürgen Willi Sievers <JSievers@NadiSoft.de>
 */
#include "rnetframemodel.h"

#include <QString>
#include <QBrush>

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
    if (parent.isValid())
        return 0;

    return ColumnCount;
}

QVariant RNetFrameModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return {};

    switch (section)
    {
    case ColTag:       return QStringLiteral("Tag");
    case ColIndex:     return QStringLiteral("#");
    case ColId:        return QStringLiteral("ID");
    case ColName:      return QStringLiteral("Name");
    case ColData:      return QStringLiteral("Data");
    case ColExt:       return QStringLiteral("Ext");
    case ColRtr:       return QStringLiteral("RTR");
    case ColTimestamp: return QStringLiteral("Timestamp");
    case ColCount:     return QStringLiteral("Count");
    case ColText:      return QStringLiteral("Text");
    default:           return {};
    }
}


Qt::ItemFlags RNetFrameModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    Qt::ItemFlags f = QAbstractTableModel::flags(index);
    if (index.column() == ColTag)
        f |= Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable;
    return f;
}

bool RNetFrameModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || index.column() != ColTag || role != Qt::CheckStateRole)
        return false;
    if (index.row() < 0 || index.row() >= static_cast<int>(m_rows.size()))
        return false;

    RowBucket &bucket = m_rows[static_cast<std::size_t>(index.row())];
    const bool tagged = value.toInt() == Qt::Checked;
    if (bucket.tagged == tagged)
        return false;

    bucket.tagged = tagged;
    emit dataChanged(index, index, {Qt::DisplayRole, Qt::CheckStateRole});
    const RNetFrame *frame = bucket.history.empty() ? nullptr : bucket.history.back().get();
    emit tagStateChanged(bucket.key, frame ? frame->name() : QStringLiteral("RNet"), bucket.tagged);
    return true;
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

    if (role == Qt::CheckStateRole && index.column() == ColTag)
        return bucket.tagged ? Qt::Checked : Qt::Unchecked;

    if (role == Qt::BackgroundRole && index.column() == ColTag && bucket.tagged)
        return QBrush(QColor("#ede7f6"));

    if (role == Qt::TextAlignmentRole)
    {
        if (index.column() == ColIndex ||
            index.column() == ColExt ||
            index.column() == ColRtr ||
            index.column() == ColTimestamp ||
            index.column() == ColCount)
        {
            return QVariant::fromValue(int(Qt::AlignRight | Qt::AlignVCenter));
        }

        return QVariant::fromValue(int(Qt::AlignLeft | Qt::AlignVCenter));
    }

    if (role == SortRole)
    {
        switch (index.column())
        {
        case ColTag:
            return bucket.tagged ? 1 : 0;

        case ColIndex:
            return index.row();

        case ColId:
            return QVariant::fromValue<qulonglong>(frame->id);

        case ColName:
            return frame->name();

        case ColData:
            return formatPayload(frame->data);

        case ColExt:
            return frame->extended ? 1 : 0;

        case ColRtr:
            return frame->remote ? 1 : 0;

        case ColTimestamp:
            return QVariant::fromValue<qulonglong>(frame->hwTimestamp);

        case ColCount:
            return static_cast<int>(bucket.history.size());

        case ColText:
            return frame->toString();

        default:
            return {};
        }
    }

    if (role != Qt::DisplayRole)
        return {};

    switch (index.column())
    {
    case ColTag:
        return {};

    case ColIndex:
        return index.row() + 1;

    case ColId:
        return QStringLiteral("0x%1")
            .arg(frame->id, frame->extended ? 8 : 3, 16, QLatin1Char('0'))
            .toUpper();

    case ColName:
        return frame->name();

    case ColData:
        return formatPayload(frame->data);

    case ColExt:
        return frame->extended ? QStringLiteral("1") : QStringLiteral("0");

    case ColRtr:
        return frame->remote ? QStringLiteral("1") : QStringLiteral("0");

    case ColTimestamp:
        return QString::number(frame->hwTimestamp, 'f', 6);

    case ColCount:
        return static_cast<int>(bucket.history.size());

    case ColText:
        return frame->toString();

    default:
        return {};
    }
}

void RNetFrameModel::clear()
{
    for (const RowBucket &bucket : m_rows)
    {
        if (!bucket.tagged)
            continue;
        const RNetFrame *frame = bucket.history.empty() ? nullptr : bucket.history.back().get();
        emit tagStateChanged(bucket.key, frame ? frame->name() : QStringLiteral("RNet"), false);
    }
    beginResetModel();
    m_rows.clear();
    m_rowByKey.clear();
    endResetModel();
}

void RNetFrameModel::addFrame(const CanFrame &frame)
{
    auto decoded = RNetFrame::decodeRNetMessage(frame);
    if (!decoded)
        return;

    const quint64 key = decoded->getKey();
    auto it = m_rowByKey.find(key);

    if (it == m_rowByKey.end())
    {
        const int row = static_cast<int>(m_rows.size());

        beginInsertRows(QModelIndex(), row, row);

        RowBucket bucket;
        bucket.key = key;
        bucket.history.push_back(std::move(decoded));

        m_rows.push_back(std::move(bucket));
        m_rowByKey.insert(key, row);

        endInsertRows();
    }
    else
    {
        const int row = it.value();
        if (row < 0 || row >= static_cast<int>(m_rows.size()))
            return;

        RowBucket &bucket = m_rows[static_cast<std::size_t>(row)];
        bucket.history.push_back(std::move(decoded));

        emit dataChanged(index(row, 0),
                         index(row, ColumnCount - 1),
                         {Qt::DisplayRole, Qt::CheckStateRole});
    }

    const int row = (it == m_rowByKey.end()) ? static_cast<int>(m_rows.size()) - 1 : it.value();
    if (row >= 0 && row < static_cast<int>(m_rows.size()))
    {
        const RowBucket &bucket = m_rows[static_cast<std::size_t>(row)];
        if (bucket.tagged)
        {
            const RNetFrame *latest = bucket.history.empty() ? nullptr : bucket.history.back().get();
            emit taggedFrameReceived(bucket.key, latest ? latest->name() : QStringLiteral("RNet"), frame);
        }
    }
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

QString RNetFrameModel::formatPayload(const QByteArray &data)
{
    if (data.isEmpty())
        return QStringLiteral("-");

    QString out;
    for (int i = 0; i < data.size(); ++i)
    {
        if (i)
            out += QLatin1Char(' ');

        out += QStringLiteral("%1")
                   .arg(static_cast<quint8>(data.at(i)), 2, 16, QLatin1Char('0'))
                   .toUpper();
    }

    return out;
}


bool RNetFrameModel::isTaggedKey(quint64 key) const
{
    auto it = m_rowByKey.constFind(key);
    if (it == m_rowByKey.constEnd())
        return false;
    const int row = it.value();
    return row >= 0 && row < static_cast<int>(m_rows.size()) && m_rows[static_cast<std::size_t>(row)].tagged;
}

QVector<CanFrame> RNetFrameModel::historyFramesForKey(quint64 key) const
{
    QVector<CanFrame> frames;
    const auto it = m_rowByKey.constFind(key);
    if (it == m_rowByKey.cend())
        return frames;
    const int row = it.value();
    if (row < 0 || row >= static_cast<int>(m_rows.size()))
        return frames;
    const RowBucket &bucket = m_rows[static_cast<std::size_t>(row)];
    frames.reserve(static_cast<int>(bucket.history.size()));
    for (const auto &entry : bucket.history)
    {
        if (entry)
            frames.push_back(static_cast<const CanFrame &>(*entry));
    }
    return frames;
}
