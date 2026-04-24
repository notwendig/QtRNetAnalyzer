#pragma once

#include "canframe.h"
#include "signalsample.h"

#include <QHash>
#include <QSet>
#include <QVector>

struct SignalHistory
{
    quint64 sourceKey = 0;
    QString sourceName;
    QString name;
    QString unit;
    QVector<SignalSample> samples;
    bool enabled = true;
};

class SignalHistoryModel
{
public:
    void clear();
    void addSample(const SignalSample &sample);
    void addSamplesFromFrame(quint64 sourceKey, const QString &sourceName, const CanFrame &frame);
    void removeSource(quint64 sourceKey);

    const QHash<quint64, SignalHistory> &allSignals() const { return m_signals; }
    QHash<quint64, SignalHistory> &allSignals() { return m_signals; }

    bool setSignalEnabled(quint64 key, bool enabled);
    bool isSignalEnabled(quint64 key) const;

    double minTime() const { return m_minTime; }
    double maxTime() const { return m_maxTime; }
    bool hasSamples() const { return m_hasTime; }

private:
    static quint8 u8(const QByteArray &data, int index);
    static qint8 s8(const QByteArray &data, int index);
    static quint16 le16(const QByteArray &data, int index);
    static quint32 le32(const QByteArray &data, int index);
    static double frameTimeSec(const CanFrame &frame);
    static quint64 makeSignalKey(quint64 sourceKey, quint8 parameterIndex);
    void recomputeTimeRange();
    static constexpr int kMaxSamplesPerSignal = 20000;

private:
    QHash<quint64, SignalHistory> m_signals;
    QHash<quint64, QString> m_sourceNames;
    double m_minTime = 0.0;
    double m_maxTime = 0.0;
    bool m_hasTime = false;
};
