// SPDX-License-Identifier: GPL-3.0-only
/*
 * QtRNetAnalyzer
 *
 * Copyright (c) 2026
 * ChatGPT (GPT-5.4 Thinking)
 * Jürgen Willi Sievers <JSievers@NadiSoft.de>
 */
#include "liveframemodel.h"

#include <QBrush>
#include <QColor>

const QStringList LiveFrameModel::m_Header = {
    QStringLiteral("Host Time"),
    QStringLiteral("HW TS"),
    QStringLiteral("CH"),
    QStringLiteral("Dir"),
    QStringLiteral("ID"),
    QStringLiteral("Std/Ext"),
    QStringLiteral("Type"),
    QStringLiteral("DLC"),
    QStringLiteral("Data")
};

LiveFrameModel::LiveFrameModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

int LiveFrameModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_frames.size();
}

int LiveFrameModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return ColCount;
}

QVariant LiveFrameModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return {};

    if (index.row() < 0 || index.row() >= m_frames.size())
        return {};

    const CanFrame &frame = m_frames.at(index.row());

    if (role == Qt::DisplayRole)
    {
        switch (index.column())
        {
        case ColHostTime:
            return frame.hostTime;

        case ColHwTimestamp:
            return QString::number(frame.hwTimestamp);

        case ColChannel:
            return QString::number(frame.channel);

        case ColDirection:
            return frame.direction == dir_tx ? QStringLiteral("TX") : QStringLiteral("RX");

        case ColId:
            return formatId(frame);

        case ColStdExt:
            return formatStdExt(frame);

        case ColType:
            return formatType(frame);

        case ColDlc:
            return QString::number(frame.data.size());

        case ColData:
            return formatData(frame.data);

        default:
            return {};
        }
    }

    if (role == SortRole)
    {
        switch (index.column())
        {
        case ColHostTime:
            return frame.hostTime;
        case ColHwTimestamp:
            return QVariant::fromValue<qulonglong>(frame.hwTimestamp);
        case ColChannel:
            return frame.channel;
        case ColDirection:
            return frame.direction == dir_tx ? 1 : 0;
        case ColId:
            return QVariant::fromValue<qulonglong>(frame.id);
        case ColStdExt:
            return frame.extended ? 1 : 0;
        case ColType:
            return frame.error ? 2 : (frame.remote ? 1 : 0);
        case ColDlc:
            return frame.data.size();
        case ColData:
            return formatData(frame.data);
        default:
            return {};
        }
    }

    if (role == Qt::TextAlignmentRole)
    {
        switch (index.column())
        {
        case ColHwTimestamp:
        case ColChannel:
        case ColDlc:
            return QVariant::fromValue(int(Qt::AlignRight | Qt::AlignVCenter));
        default:
            return QVariant::fromValue(int(Qt::AlignLeft | Qt::AlignVCenter));
        }
    }

    if (role == Qt::ForegroundRole)
    {
        if (frame.error)
            return QBrush(QColor(180, 0, 0));
    }

    if (role == Qt::BackgroundRole)
    {
        if (frame.error)
            return QBrush(QColor(255, 230, 230));

        if (frame.remote)
            return QBrush(QColor(245, 245, 210));

        if (frame.extended)
            return QBrush(QColor(235, 245, 255));
    }

    if (role == Qt::ToolTipRole)
    {
        return QStringLiteral("ID=%1\nType=%2\nBytes=%3")
        .arg(formatId(frame))
            .arg(formatType(frame))
            .arg(formatData(frame.data));
    }

    return {};
}

bool LiveFrameModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    Q_UNUSED(index)
    Q_UNUSED(value)
    Q_UNUSED(role)
    return false;
}

QVariant LiveFrameModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return {};

    if (orientation == Qt::Horizontal)
    {
        if (section >= 0 && section < m_Header.size())
            return m_Header.at(section);
        return {};
    }

    return section + 1;
}

Qt::ItemFlags LiveFrameModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

void LiveFrameModel::clear()
{
    beginResetModel();
    m_frames.clear();
    endResetModel();
}

void LiveFrameModel::setMaxRows(int maxRows)
{
    if (maxRows < 1)
        maxRows = 1;

    if (m_maxRows == maxRows)
        return;

    m_maxRows = maxRows;

    if (m_frames.size() > m_maxRows)
    {
        const int removeCount = m_frames.size() - m_maxRows;
        beginRemoveRows(QModelIndex(), 0, removeCount - 1);
        m_frames.erase(m_frames.begin(), m_frames.begin() + removeCount);
        endRemoveRows();
    }
}

void LiveFrameModel::addFrame(const CanFrame &frame)
{
    appendFrame(frame);
}

void LiveFrameModel::addFrames(const QVector<CanFrame> &frames)
{
    if (frames.isEmpty())
        return;

    for (const CanFrame &frame : frames)
        appendFrame(frame);
}

const CanFrame *LiveFrameModel::frameAt(int row) const
{
    if (row < 0 || row >= m_frames.size())
        return nullptr;

    return &m_frames.at(row);
}

void LiveFrameModel::appendFrame(const CanFrame &frame)
{
    const int newRow = m_frames.size();
    beginInsertRows(QModelIndex(), newRow, newRow);
    m_frames.push_back(frame);
    endInsertRows();

    if (m_frames.size() > m_maxRows)
    {
        beginRemoveRows(QModelIndex(), 0, 0);
        m_frames.removeFirst();
        endRemoveRows();
    }
}

QString LiveFrameModel::formatId(const CanFrame &frame)
{
    return QStringLiteral("0x%1")
    .arg(frame.id, frame.extended ? 8 : 3, 16, QLatin1Char('0'))
        .toUpper();
}

QString LiveFrameModel::formatStdExt(const CanFrame &frame)
{
    return frame.extended ? QStringLiteral("EXT") : QStringLiteral("STD");
}

QString LiveFrameModel::formatType(const CanFrame &frame)
{
    if (frame.error)
        return QStringLiteral("ERR");

    return frame.remote ? QStringLiteral("RTR") : QStringLiteral("DATA");
}

QString LiveFrameModel::formatData(const QByteArray &data)
{
    if (data.isEmpty())
        return QStringLiteral("-");

    return data.toHex(' ').toUpper();
}
