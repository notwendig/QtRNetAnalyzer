#pragma once

#include <QAbstractTableModel>
#include <QByteArray>
#include <QElapsedTimer>
#include <QHash>
#include <QSet>
#include <QString>
#include <QVector>

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
        ColPlot = 0,
        ColIndex = 1,
        ColCount = 2,
        ColId = 3,
        ColName = 4,
        ColIdParts = 5,
        ColFields = 6,
        ColData = 7,
        ColExt = 8,
        ColRtr = 9,
        ColTimestamp = 10,
        ColumnCount = 11
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
    struct RowBucket {
        quint64 key = 0;
        quint64 totalCount = 0;
        std::vector<std::unique_ptr<RNetFrame>> history;
        QElapsedTimer updateThrottle;
        bool throttleStarted = false;
    };

private:
    static QString formatPayload(const QByteArray &data);
    static QString idPartsString(const RNetFrame &frame);
    static QString fieldsString(const RNetFrame &frame);
    static QString sortString(QString value);

    static constexpr std::size_t kMaxHistoryPerRow = 2000;
    static constexpr qint64 kUiUpdateIntervalMs = 80;

private:
    std::vector<RowBucket> m_rows;
    QHash<quint64, int> m_rowByKey;
    QSet<quint64> m_taggedKeys;
};
