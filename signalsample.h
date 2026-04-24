#pragma once

#include <QString>
#include <QtGlobal>

struct SignalSample
{
    quint64 key = 0;        // unique signal key: selected R-Net message key + parameter index
    quint64 sourceKey = 0;  // selected R-Net table row key
    QString name;
    double timeSec = 0.0;
    double value = 0.0;
    QString unit;

    SignalSample() = default;

    SignalSample(quint64 signalKey,
                 quint64 rnetSourceKey,
                 const QString &signalName,
                 double timestampSec,
                 double signalValue,
                 const QString &signalUnit = {})
        : key(signalKey)
        , sourceKey(rnetSourceKey)
        , name(signalName)
        , timeSec(timestampSec)
        , value(signalValue)
        , unit(signalUnit)
    {
    }
};
