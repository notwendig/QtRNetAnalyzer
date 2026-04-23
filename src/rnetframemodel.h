// SPDX-License-Identifier: GPL-3.0-only
/*
 * QtRNetAnalyzer
 *
 * Copyright (c) 2026
 * ChatGPT (GPT-5.4 Thinking)
 * Jürgen Willi Sievers <JSievers@NadiSoft.de>
 */
#pragma once

#include <QAbstractTableModel>
#include <QByteArray>
#include <QHash>
#include <QString>

#include <memory>
#include <vector>

#include "canframe.h"
#include "rnetframe.h"

class RNetFrameModel final : public QAbstractTableModel
{
    Q_OBJECT

  public:
    explicit RNetFrameModel(QObject *parent = nullptr);

    enum Roles
    {
        SortRole = Qt::UserRole
    };
    ~RNetFrameModel() override = default;

    enum Column
    {
        ColTag = 0,
        ColIndex,
        ColId,
        ColName,
        ColData,
        ColExt,
        ColRtr,
        ColTimestamp,
        ColCount,
        ColText,
        ColumnCount
    };

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    void clear();
    void addFrame(const CanFrame &frame);

    const RNetFrame *latestFrameAt(int row) const;
    const std::vector<std::unique_ptr<RNetFrame>> *historyAt(int row) const;
    QVector<CanFrame> historyFramesForKey(quint64 key) const;
    bool isTaggedKey(quint64 key) const;

signals:
    void tagStateChanged(quint64 key, const QString &name, bool tagged);
    void taggedFrameReceived(quint64 key, const QString &name, const CanFrame &frame);

  private:
    struct RowBucket
    {
        quint64 key = 0;
        bool tagged = false;
        std::vector<std::unique_ptr<RNetFrame>> history;
    };

  private:
    // std::unique_ptr<RNetFrame> decodeRNetMessage(const CanFrame &frame) const;
    // quint64 makeKey(const RNetFrame &frame) const;
    static QString formatPayload(const QByteArray &data);

    // quint8 u8(const QByteArray &data, int i) const
    // {
    //     if (i < 0 || i >= data.size())
    //         return 0;
    //     return static_cast<quint8>(data.at(i));
    // }

  private:
    std::vector<RowBucket> m_rows;
    QHash<quint64, int> m_rowByKey;
};
