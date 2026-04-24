#pragma once

#include <QAbstractTableModel>
#include <QVector>
#include <QStringList>

#include "canframe.h"

class LiveFrameModel final : public QAbstractTableModel
{
    Q_OBJECT

  public:
    enum Column
    {
        ColHostTime = 0,
        ColHwTimestamp,
        ColChannel,
        ColDirection,
        ColId,
        ColStdExt,
        ColType,
        ColDlc,
        ColData,
        ColCount
    };

    explicit LiveFrameModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;

    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    void clear();
    void setMaxRows(int maxRows);
    void addFrame(const CanFrame &frame);
    void addFrames(const QVector<CanFrame> &frames);

    const CanFrame *frameAt(int row) const;

  private:
    void appendFrame(const CanFrame &frame);
    static QString formatId(const CanFrame &frame);
    static QString formatStdExt(const CanFrame &frame);
    static QString formatType(const CanFrame &frame);
    static QString formatData(const QByteArray &data);

  private:
    static const QStringList m_Header;
    QVector<CanFrame> m_frames;
    int m_maxRows = 5000;
};
