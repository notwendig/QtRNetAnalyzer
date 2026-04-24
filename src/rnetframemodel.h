#pragma once

#include <QAbstractTableModel>
#include <QByteArray>
#include <QHash>
#include <QSet>
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
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    void clear();
    void addFrame(const CanFrame &frame);

    const RNetFrame *latestFrameAt(int row) const;
    const std::vector<std::unique_ptr<RNetFrame>> *historyAt(int row) const;
    const std::vector<std::unique_ptr<RNetFrame>> *historyForKey(quint64 key) const;
    QString nameForKey(quint64 key) const;
    bool isTagged(quint64 key) const { return m_taggedKeys.contains(key); }

  signals:
    void tagStateChanged(quint64 key, const QString &name, bool enabled);
    void taggedFrameReceived(quint64 key, const QString &name, const CanFrame &frame);

  private:
    struct RowBucket
    {
        quint64 key = 0;
        std::vector<std::unique_ptr<RNetFrame>> history;
    };

  private:
    static QString formatPayload(const QByteArray &data);

  private:
    std::vector<RowBucket> m_rows;
    QHash<quint64, int> m_rowByKey;
    QSet<quint64> m_taggedKeys;
};
