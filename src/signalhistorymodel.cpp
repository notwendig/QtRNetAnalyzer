#include "signalhistorymodel.h"

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
}

void SignalHistoryModel::clear()
{
    m_signals.clear();
    m_sourceNames.clear();
    m_minTime = 0.0;
    m_maxTime = 0.0;
    m_hasTime = false;
}

void SignalHistoryModel::addSample(const SignalSample &sample)
{
    auto &history = m_signals[sample.key];
    if (history.name.isEmpty()) {
        history.sourceKey = sample.sourceKey;
        history.sourceName = m_sourceNames.value(sample.sourceKey);
        history.name = sample.name;
        history.unit = sample.unit;
    }

    history.samples.push_back(sample);
    if (history.samples.size() > kMaxSamplesPerSignal)
        history.samples.erase(history.samples.begin(), history.samples.begin() + (history.samples.size() - kMaxSamplesPerSignal));

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
    if (sourceKey == 0 || frame.remote || frame.error)
        return;

    const double t = frameTimeSec(frame);
    const quint32 id = frame.id;
    const QByteArray &d = frame.data;
    const QString prefix = sourceName.isEmpty() ? QStringLiteral("R-Net") : sourceName;
    m_sourceNames.insert(sourceKey, prefix);

    // Joystick position: 02000M00#XxYy, signed 8-bit, periodic ~10 ms.
    if (frame.extended && ((id & 0x2FFF0FFFu) == 0x02000000u || id == kJoystickId) && d.size() >= 2) {
        addSample(SignalSample(makeSignalKey(sourceKey, 0), sourceKey, prefix + QStringLiteral(" X"), t, s8(d, 0), QStringLiteral("%")));
        addSample(SignalSample(makeSignalKey(sourceKey, 1), sourceKey, prefix + QStringLiteral(" Y"), t, s8(d, 1), QStringLiteral("%")));
        return;
    }

    // Battery level: 1C0C0X00#Pp, percent.
    if (frame.extended && (id & 0xFFFFF0FFu) == kBatteryBase && d.size() >= 1) {
        addSample(SignalSample(makeSignalKey(sourceKey, 0), sourceKey, prefix, t, u8(d, 0), QStringLiteral("%")));
        return;
    }

    // Drive motor current/power: 14300X00#LlHh, little-endian raw value.
    if (frame.extended && (id & 0xFFFFF0FFu) == kMotorCurrentBase && d.size() >= 2) {
        addSample(SignalSample(makeSignalKey(sourceKey, 0), sourceKey, prefix, t, le16(d, 0), QStringLiteral("raw")));
        return;
    }

    // Distance / amp-hour counter: 1C300X04#LLLLLLLLRRRRRRRR, two LE32 channels.
    if (frame.extended && (id & 0xFFFFF0FFu) == kDistanceBase && d.size() >= 8) {
        addSample(SignalSample(makeSignalKey(sourceKey, 0), sourceKey, prefix + QStringLiteral(" L"), t, le32(d, 0), QStringLiteral("raw")));
        addSample(SignalSample(makeSignalKey(sourceKey, 1), sourceKey, prefix + QStringLiteral(" R"), t, le32(d, 4), QStringLiteral("raw")));
        return;
    }

    // Motor speed max / power attribution: 0A040X00#Pp.
    if (frame.extended && (id & 0xFFFFF0FFu) == kMotorMaxSpeedBase && d.size() >= 1) {
        addSample(SignalSample(makeSignalKey(sourceKey, 0), sourceKey, prefix, t, u8(d, 0), QStringLiteral("%")));
        return;
    }

    // PM heartbeat/status byte: 0C140X00#Xx.
    if (frame.extended && (id & 0xFFFFF0FFu) == kPmHeartbeatBase && d.size() >= 1) {
        addSample(SignalSample(makeSignalKey(sourceKey, 0), sourceKey, prefix, t, u8(d, 0), QStringLiteral("raw")));
        return;
    }

    // Lamp status bitmap: 0C000E00#MaskBitmap.
    if (frame.extended && (id & 0xFFFFF0FFu) == kLampStatusBase && d.size() >= 2) {
        addSample(SignalSample(makeSignalKey(sourceKey, 0), sourceKey, prefix + QStringLiteral(" Mask"), t, u8(d, 0), QStringLiteral("raw")));
        addSample(SignalSample(makeSignalKey(sourceKey, 1), sourceKey, prefix + QStringLiteral(" Bitmap"), t, u8(d, 1), QStringLiteral("raw")));
        return;
    }

    // Enable/mode output family: useful for mode-change traces.
    if (frame.extended && (id & 0xFFFFF000u) == kEnableMotorOutputBase && d.size() >= 1) {
        addSample(SignalSample(makeSignalKey(sourceKey, 0), sourceKey, prefix, t, u8(d, 0), QStringLiteral("raw")));
        return;
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

    recomputeTimeRange();
}

bool SignalHistoryModel::setSignalEnabled(quint64 key, bool enabled)
{
    auto it = m_signals.find(key);
    if (it == m_signals.end())
        return false;
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
    return double(frame.hwTimestamp) / 1000.0;
}

quint64 SignalHistoryModel::makeSignalKey(quint64 sourceKey, quint8 parameterIndex)
{
    return (sourceKey << 8) | quint64(parameterIndex);
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
