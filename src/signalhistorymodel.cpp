#include "signalhistorymodel.h"

#include <QElapsedTimer>
#include <QList>
#include <QtGlobal>

namespace {

constexpr quint32 kJoystickId = 0x02000300u;
constexpr quint32 kBatteryBase = 0x1C0C0000u;
constexpr quint32 kMotorCurrentBase = 0x14300000u;
constexpr quint32 kDistanceBase = 0x1C300004u;
constexpr quint32 kMotorMaxSpeedBase = 0x0A040000u;
constexpr quint32 kPmHeartbeatBase = 0x0C140000u;
constexpr quint32 kLampStatusBase = 0x0C000E00u;
constexpr quint32 kEnableMotorOutputBase = 0x0C180000u;

constexpr quint16 kSignalFrameCount = 0;
constexpr quint16 kSignalKnownBase = 10;
constexpr quint16 kSignalPayloadBase = 100;

} // namespace

void SignalHistoryModel::clear()
{
    m_signals.clear();
    m_sourceNames.clear();
    m_sourceCounts.clear();
    m_minTime = 0.0;
    m_maxTime = 0.0;
    m_hasTime = false;
    m_fallbackBaseSec = -1.0;
    m_lastFallbackTimeSec = -1.0;
}

void SignalHistoryModel::addSample(const SignalSample &sample)
{
    addSampleInternal(sample, true, true);
}

void SignalHistoryModel::addSampleInternal(const SignalSample &sample, bool enabledByDefault, bool plottable)
{
    auto it = m_signals.find(sample.key);
    if (it == m_signals.end()) {
        SignalHistory history;
        history.sourceKey = sample.sourceKey;
        history.sourceName = m_sourceNames.value(sample.sourceKey);
        history.name = sample.name;
        history.unit = sample.unit;
        history.enabled = enabledByDefault;
        history.plottable = plottable;
        it = m_signals.insert(sample.key, history);
    } else {
        // Preserve the user's checkbox state for existing signals, but keep
        // metadata locked out of plotting when later samples arrive.
        it->plottable = plottable;
        if (!plottable)
            it->enabled = false;
        if (it->name.isEmpty())
            it->name = sample.name;
        if (it->unit.isEmpty())
            it->unit = sample.unit;
        if (it->sourceName.isEmpty())
            it->sourceName = m_sourceNames.value(sample.sourceKey);
    }

    it->samples.push_back(sample);
    if (it->samples.size() > kMaxSamplesPerSignal) {
        it->samples.erase(it->samples.begin(),
                          it->samples.begin() + (it->samples.size() - kMaxSamplesPerSignal));
    }

    if (!m_hasTime) {
        m_minTime = sample.timeSec;
        m_maxTime = sample.timeSec;
        m_hasTime = true;
        return;
    }

    m_minTime = qMin(m_minTime, sample.timeSec);
    m_maxTime = qMax(m_maxTime, sample.timeSec);
}

void SignalHistoryModel::addSamplesFromFrame(quint64 sourceKey, const QString &sourceName, const CanFrame &frame)
{
    if (sourceKey == 0 || frame.error)
        return;

    const QString prefix = sourceName.isEmpty() ? QStringLiteral("R-Net") : sourceName;
    m_sourceNames.insert(sourceKey, prefix);

    // Keep the counter visible in the tree/value column, but do not plot it by
    // default. Otherwise the monotonically increasing frame count dominates the
    // Y range and makes joystick/battery signals look like a broken flat line.
    const double t = frameTimeSec(frame);
    addFrameCounterSample(sourceKey, prefix, t);

    if (frame.remote)
        return;

    const quint32 id = frame.id;
    const QByteArray &d = frame.data;
    bool producedKnownSignal = false;

    // Joystick position: 02000M00#XxYy, signed 8-bit, periodic ~10 ms.
    if (frame.extended && (((id & 0xFFFFF0FFu) == 0x02000000u) || id == kJoystickId) && d.size() >= 2) {
        addSample(SignalSample(makeSignalKey(sourceKey, kSignalKnownBase + 0), sourceKey,
                               prefix + QStringLiteral(" X"), t, s8(d, 0), QStringLiteral("%")));
        addSample(SignalSample(makeSignalKey(sourceKey, kSignalKnownBase + 1), sourceKey,
                               prefix + QStringLiteral(" Y"), t, s8(d, 1), QStringLiteral("%")));
        producedKnownSignal = true;
    }

    // Battery level: 1C0C0X00#Pp, percent.
    if (frame.extended && (id & 0xFFFFF0FFu) == kBatteryBase && d.size() >= 1) {
        addSample(SignalSample(makeSignalKey(sourceKey, kSignalKnownBase + 0), sourceKey,
                               prefix + QStringLiteral(" %"), t, u8(d, 0), QStringLiteral("%")));
        producedKnownSignal = true;
    }

    // Drive motor current/power: 14300X00#LlHh, little-endian raw value.
    if (frame.extended && (id & 0xFFFFF0FFu) == kMotorCurrentBase && d.size() >= 2) {
        addSample(SignalSample(makeSignalKey(sourceKey, kSignalKnownBase + 0), sourceKey,
                               prefix + QStringLiteral(" Current"), t, le16(d, 0), QStringLiteral("raw")));
        producedKnownSignal = true;
    }

    // Distance / amp-hour counter: 1C300X04#LLLLLLLLRRRRRRRR, two LE32 channels.
    if (frame.extended && (id & 0xFFFFF0FFu) == kDistanceBase && d.size() >= 8) {
        addSample(SignalSample(makeSignalKey(sourceKey, kSignalKnownBase + 0), sourceKey,
                               prefix + QStringLiteral(" L"), t, le32(d, 0), QStringLiteral("raw")));
        addSample(SignalSample(makeSignalKey(sourceKey, kSignalKnownBase + 1), sourceKey,
                               prefix + QStringLiteral(" R"), t, le32(d, 4), QStringLiteral("raw")));
        producedKnownSignal = true;
    }

    // Motor speed max / power attribution: 0A040X00#Pp.
    if (frame.extended && (id & 0xFFFFF0FFu) == kMotorMaxSpeedBase && d.size() >= 1) {
        addSample(SignalSample(makeSignalKey(sourceKey, kSignalKnownBase + 0), sourceKey,
                               prefix + QStringLiteral(" Max Speed"), t, u8(d, 0), QStringLiteral("%")));
        producedKnownSignal = true;
    }

    // PM heartbeat/status byte: 0C140X00#Xx.
    if (frame.extended && (id & 0xFFFFF0FFu) == kPmHeartbeatBase && d.size() >= 1) {
        addSample(SignalSample(makeSignalKey(sourceKey, kSignalKnownBase + 0), sourceKey,
                               prefix + QStringLiteral(" Status"), t, u8(d, 0), QStringLiteral("raw")));
        producedKnownSignal = true;
    }

    // Lamp status bitmap: 0C000E00#MaskBitmap.
    if (frame.extended && (id & 0xFFFFF0FFu) == kLampStatusBase && d.size() >= 2) {
        addSample(SignalSample(makeSignalKey(sourceKey, kSignalKnownBase + 0), sourceKey,
                               prefix + QStringLiteral(" Mask"), t, u8(d, 0), QStringLiteral("raw")));
        addSample(SignalSample(makeSignalKey(sourceKey, kSignalKnownBase + 1), sourceKey,
                               prefix + QStringLiteral(" Bitmap"), t, u8(d, 1), QStringLiteral("raw")));
        producedKnownSignal = true;
    }

    // Enable/mode output family: useful for mode-change traces.
    if (frame.extended && (id & 0xFFFFF000u) == kEnableMotorOutputBase && d.size() >= 1) {
        addSample(SignalSample(makeSignalKey(sourceKey, kSignalKnownBase + 0), sourceKey,
                               prefix + QStringLiteral(" Mode"), t, u8(d, 0), QStringLiteral("raw")));
        producedKnownSignal = true;
    }

    // Fallback: for every selected R-Net message that has no specific signal
    // mapping yet, plot each payload byte. This is essential for reverse
    // engineering because selected/unknown R-Net frames must still become visible.
    if (!producedKnownSignal)
        addPayloadByteSamples(sourceKey, prefix, frame, t);
}

void SignalHistoryModel::addFrameCounterSample(quint64 sourceKey, const QString &sourceName, double t)
{
    const quint64 count = ++m_sourceCounts[sourceKey];
    addSampleInternal(SignalSample(makeSignalKey(sourceKey, kSignalFrameCount), sourceKey,
                                   sourceName + QStringLiteral(" Count"), t,
                                   static_cast<double>(count), QStringLiteral("frames")),
                      false,
                      false);
}

void SignalHistoryModel::addPayloadByteSamples(quint64 sourceKey, const QString &sourceName, const CanFrame &frame, double t)
{
    for (int i = 0; i < frame.data.size() && i < 8; ++i) {
        addSample(SignalSample(makeSignalKey(sourceKey, kSignalPayloadBase + quint16(i)), sourceKey,
                               sourceName + QStringLiteral(" Byte %1").arg(i), t,
                               u8(frame.data, i), QStringLiteral("raw")));
    }
}

void SignalHistoryModel::removeSource(quint64 sourceKey)
{
    QList<quint64> removeKeys;
    for (auto it = m_signals.constBegin(); it != m_signals.constEnd(); ++it) {
        if (it.value().sourceKey == sourceKey)
            removeKeys.push_back(it.key());
    }

    for (const quint64 key : removeKeys)
        m_signals.remove(key);

    m_sourceNames.remove(sourceKey);
    m_sourceCounts.remove(sourceKey);
    recomputeTimeRange();
}

bool SignalHistoryModel::setSignalEnabled(quint64 key, bool enabled)
{
    auto it = m_signals.find(key);
    if (it == m_signals.end())
        return false;

    if (!it->plottable) {
        it->enabled = false;
        return false;
    }

    it->enabled = enabled;
    return true;
}

bool SignalHistoryModel::isSignalEnabled(quint64 key) const
{
    auto it = m_signals.constFind(key);
    return it == m_signals.constEnd() ? false : it->enabled;
}

quint8 SignalHistoryModel::u8(const QByteArray &data, int index)
{
    if (index < 0 || index >= data.size())
        return 0;
    return static_cast<quint8>(data.at(index));
}

qint8 SignalHistoryModel::s8(const QByteArray &data, int index)
{
    return static_cast<qint8>(u8(data, index));
}

quint16 SignalHistoryModel::le16(const QByteArray &data, int index)
{
    return quint16(u8(data, index)) | (quint16(u8(data, index + 1)) << 8);
}

quint32 SignalHistoryModel::le32(const QByteArray &data, int index)
{
    return quint32(u8(data, index)) |
           (quint32(u8(data, index + 1)) << 8) |
           (quint32(u8(data, index + 2)) << 16) |
           (quint32(u8(data, index + 3)) << 24);
}

double SignalHistoryModel::frameTimeSec(const CanFrame &frame)
{
    if (frame.hwTimestamp != 0)
        return double(frame.hwTimestamp) / 1000.0;

    // SocketCAN frames currently arrive without a hardware timestamp in the
    // QtRNetAnalyzer worker. Also, when an already captured/tagged row is added
    // to the Signal View, many historical frames can be replayed into the model
    // during one GUI event. Therefore a plain wall-clock fallback is not enough:
    // keep the fallback monotonic and advance it by at least 1 ms per frame.
    static QElapsedTimer fallbackClock;
    if (!fallbackClock.isValid())
        fallbackClock.start();

    const double nowSec = double(fallbackClock.elapsed()) / 1000.0;
    if (m_fallbackBaseSec < 0.0)
        m_fallbackBaseSec = nowSec;

    double t = nowSec - m_fallbackBaseSec;
    if (m_lastFallbackTimeSec >= 0.0 && t <= m_lastFallbackTimeSec)
        t = m_lastFallbackTimeSec + 0.001;

    m_lastFallbackTimeSec = t;
    return t;
}

quint64 SignalHistoryModel::makeSignalKey(quint64 sourceKey, quint16 parameterIndex)
{
    return (sourceKey << 16) | quint64(parameterIndex);
}

void SignalHistoryModel::recomputeTimeRange()
{
    m_hasTime = false;
    m_minTime = 0.0;
    m_maxTime = 0.0;

    for (const SignalHistory &history : m_signals) {
        for (const SignalSample &sample : history.samples) {
            if (!m_hasTime) {
                m_minTime = sample.timeSec;
                m_maxTime = sample.timeSec;
                m_hasTime = true;
            } else {
                m_minTime = qMin(m_minTime, sample.timeSec);
                m_maxTime = qMax(m_maxTime, sample.timeSec);
            }
        }
    }
}
