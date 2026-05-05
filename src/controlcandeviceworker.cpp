#include "controlcandeviceworker.h"

#include <QByteArray>
#include <QDateTime>
#include <QMutexLocker>

#include <algorithm>
#include <chrono>
#include <vector>

namespace {

constexpr int kMaxCanPayloadBytes = 8;

} // namespace

ControlCanDeviceWorker::ControlCanDeviceWorker(QObject *parent)
    : QThread(parent)
{
}

ControlCanDeviceWorker::~ControlCanDeviceWorker()
{
    closeDevice();
}

QString ControlCanDeviceWorker::resultToString(long result) const
{
#if QTRA_HAS_WAVESHARE_USBCANB
    if (!m_device.lastError().empty())
        return QString::fromStdString(m_device.lastError());
#endif

    if (result == 1)
        return QStringLiteral("OK");
    if (result == 0)
        return QStringLiteral("operation failed");
    if (result == -1)
        return QStringLiteral("device missing or USB disconnected");
    return QStringLiteral("result=%1").arg(result);
}

#if QTRA_HAS_WAVESHARE_USBCANB

qusbcanb::Channel ControlCanDeviceWorker::lowLevelChannel(int channelIndex)
{
    return channelIndex == 0 ? qusbcanb::Channel::Can1 : qusbcanb::Channel::Can2;
}

qusbcanb::CanMode ControlCanDeviceWorker::lowLevelMode(UCHAR mode)
{
    switch (mode) {
    case 0:
        return qusbcanb::CanMode::Normal;
    case 1:
        return qusbcanb::CanMode::ListenOnly;
    case 2:
        return qusbcanb::CanMode::SelfTest;
    default:
        return qusbcanb::CanMode::Normal;
    }
}

std::uint32_t ControlCanDeviceWorker::bitrateFromTiming(UCHAR timing0, UCHAR timing1)
{
    const quint16 timing = (static_cast<quint16>(timing0) << 8) | timing1;
    switch (timing) {
    case 0x0014:
        return 1000000;
    case 0x001C:
        return 500000;
    case 0x011C:
        return 250000;
    case 0x031C:
        return 125000;
    case 0x041C:
        return 100000;
    case 0x091C:
        return 50000;
    default:
        return 125000;
    }
}

qusbcanb::CanFrame ControlCanDeviceWorker::toLowLevelFrame(const CanFrame &frame)
{
    qusbcanb::CanFrame out;
    out.id = frame.id;
    out.extended = frame.extended;
    out.remote = frame.remote;

    const int len = std::min<int>(static_cast<int>(frame.data.size()), kMaxCanPayloadBytes);
    out.dlc = static_cast<std::uint8_t>(len);
    for (int i = 0; i < len; ++i)
        out.data[static_cast<std::size_t>(i)] = static_cast<std::uint8_t>(frame.data.at(i));

    return out;
}

CanFrame ControlCanDeviceWorker::fromLowLevelFrame(const qusbcanb::CanFrame &frame, int channel, direction_t direction)
{
    CanFrame out;
    out.hostTime = QDateTime::currentDateTime().toString(Qt::ISODateWithMs);
    out.hwTimestamp = 0;
    out.id = frame.id;
    out.extended = frame.extended;
    out.remote = frame.remote;
    out.error = false;
    out.channel = channel;
    out.direction = direction;

    const int len = std::min<int>(static_cast<int>(frame.dlc), kMaxCanPayloadBytes);
    out.data.resize(len);
    for (int i = 0; i < len; ++i)
        out.data[i] = static_cast<char>(frame.data[static_cast<std::size_t>(i)]);

    return out;
}

bool ControlCanDeviceWorker::initChannel(const ChannelConfig &cfg, QString *errorMessage)
{
    if (!cfg.enabled)
        return true;

    const std::uint32_t bitrate = bitrateFromTiming(cfg.timing0, cfg.timing1);
    const qusbcanb::CanMode mode = lowLevelMode(cfg.mode);
    const qusbcanb::Channel channel = lowLevelChannel(static_cast<int>(cfg.canIndex));

    if (!m_device.configure(channel, bitrate, mode)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Waveshare configure CAN%1 failed: %2")
                                .arg(cfg.canIndex + 1)
                                .arg(QString::fromStdString(m_device.lastError()));
        }
        return false;
    }

    return true;
}

bool ControlCanDeviceWorker::openDevice(const DeviceOpenConfig &config, QString *errorMessage)
{
    QMutexLocker locker(&m_mutex);
    if (m_open) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Device already open");
        return false;
    }

    m_config = config;
    m_rx0 = m_rx1 = m_tx0 = m_tx1 = m_err0 = m_err1 = 0;
    m_txQueue.clear();

    qusbcanb::DeviceConfig lowConfig;
    if (!m_device.open(lowConfig)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Waveshare USBCAN-B open failed: %1")
                                .arg(QString::fromStdString(m_device.lastError()));
        }
        return false;
    }

    QString localError;
    if (!initChannel(m_config.channel0, &localError) || !initChannel(m_config.channel1, &localError)) {
        m_device.close();
        if (errorMessage)
            *errorMessage = localError;
        return false;
    }

    m_running = true;
    m_open = true;
    locker.unlock();

    start();
    emit statusMessage(QStringLiteral("Waveshare USBCAN-B opened"), false);
    emit deviceStateChanged(true);
    return true;
}

void ControlCanDeviceWorker::closeDevice()
{
    {
        QMutexLocker locker(&m_mutex);
        if (!m_open && !m_running)
            return;
        m_running = false;
        m_wait.wakeAll();
    }

    wait(1000);

    QMutexLocker locker(&m_mutex);
    m_device.close();
    m_open = false;
    m_txQueue.clear();
    locker.unlock();

    emit statusMessage(QStringLiteral("Waveshare USBCAN-B closed"), false);
    emit deviceStateChanged(false);
}

bool ControlCanDeviceWorker::isOpen() const
{
    QMutexLocker locker(&m_mutex);
    return m_open;
}

void ControlCanDeviceWorker::queueTransmit(int channel, quint32 id, const QByteArray &data, bool extended, bool remote)
{
    QMutexLocker locker(&m_mutex);
    if (!m_open) {
        emit statusMessage(QStringLiteral("Transmit ignored: device not open"), true);
        return;
    }

    CanFrame tx;
    tx.channel = channel;
    tx.id = id;
    tx.data = data.left(kMaxCanPayloadBytes);
    tx.extended = extended;
    tx.remote = remote;
    tx.direction = dir_tx;
    m_txQueue.push_back(tx);
    m_wait.wakeAll();
}

void ControlCanDeviceWorker::clearHardwareBuffers()
{
    QMutexLocker locker(&m_mutex);
    if (!m_open)
        return;

    if (m_config.channel0.enabled)
        m_device.clearRx(qusbcanb::Channel::Can1);
    if (m_config.channel1.enabled)
        m_device.clearRx(qusbcanb::Channel::Can2);

    emit statusMessage(QStringLiteral("Hardware receive buffers cleared"), false);
}

void ControlCanDeviceWorker::resetChannels()
{
    QMutexLocker locker(&m_mutex);
    if (!m_open)
        return;

    QString error;
    const bool ok0 = initChannel(m_config.channel0, &error);
    const bool ok1 = initChannel(m_config.channel1, &error);

    emit statusMessage(ok0 && ok1 ? QStringLiteral("Channels reconfigured") : error, !(ok0 && ok1));
}

CanFrame ControlCanDeviceWorker::toFrame(const VCI_CAN_OBJ &obj, int channel, direction_t direction) const
{
    CanFrame frame;
    frame.hostTime = QDateTime::currentDateTime().toString(Qt::ISODateWithMs);
    frame.hwTimestamp = obj.TimeStamp;
    frame.id = obj.ID;
    frame.extended = (obj.ExternFlag != 0);
    frame.remote = (obj.RemoteFlag != 0);
    frame.error = false;
    frame.data = QByteArray(reinterpret_cast<const char *>(obj.Data), obj.DataLen);
    frame.channel = channel;
    frame.direction = direction;
    return frame;
}

void ControlCanDeviceWorker::processPendingTx()
{
    QVector<CanFrame> queue;
    {
        QMutexLocker locker(&m_mutex);
        queue = std::move(m_txQueue);
        m_txQueue.clear();
    }

    for (const CanFrame &tx : queue) {
        const qusbcanb::Channel channel = lowLevelChannel(tx.channel);
        const qusbcanb::CanFrame frame = toLowLevelFrame(tx);

        if (m_device.send(channel, frame)) {
            if (tx.channel == 0)
                ++m_tx0;
            else
                ++m_tx1;
            emit frameTransmitted(tx);
        } else {
            if (tx.channel == 0)
                ++m_err0;
            else
                ++m_err1;
            emit statusMessage(QStringLiteral("Waveshare send CAN%1 failed: %2")
                                   .arg(tx.channel + 1)
                                   .arg(QString::fromStdString(m_device.lastError())),
                               true);
        }
    }
}

void ControlCanDeviceWorker::processRxForChannel(const ChannelConfig &cfg)
{
    if (!cfg.enabled)
        return;

    std::vector<qusbcanb::CanFrame> rawFrames;
    const std::size_t maxFrames = static_cast<std::size_t>(std::max(1, m_config.receiveBatch));
    const auto timeout = std::chrono::milliseconds{0};

    if (!m_device.receive(lowLevelChannel(static_cast<int>(cfg.canIndex)), rawFrames, maxFrames, timeout)) {
        const std::string err = m_device.lastError();
        if (!err.empty()) {
            if (cfg.canIndex == 0)
                ++m_err0;
            else
                ++m_err1;
            emit statusMessage(QStringLiteral("Waveshare receive CAN%1 failed: %2")
                                   .arg(cfg.canIndex + 1)
                                   .arg(QString::fromStdString(err)),
                               true);
        }
        return;
    }

    if (rawFrames.empty())
        return;

    QVector<CanFrame> frames;
    frames.reserve(static_cast<qsizetype>(rawFrames.size()));
    for (const qusbcanb::CanFrame &raw : rawFrames)
        frames.push_back(fromLowLevelFrame(raw, static_cast<int>(cfg.canIndex), dir_rx));

    if (cfg.canIndex == 0)
        m_rx0 += static_cast<quint64>(frames.size());
    else
        m_rx1 += static_cast<quint64>(frames.size());

    emit frameBatchReady(frames);
}

void ControlCanDeviceWorker::run()
{
    while (true) {
        {
            QMutexLocker locker(&m_mutex);
            if (!m_running)
                break;
        }

        processPendingTx();
        processRxForChannel(m_config.channel0);
        processRxForChannel(m_config.channel1);
        emit countersUpdated(m_rx0, m_rx1, m_tx0, m_tx1, m_err0, m_err1);

        QMutexLocker locker(&m_mutex);
        if (!m_running)
            break;
        m_wait.wait(&m_mutex, std::max(1, m_config.pollDelayMs));
    }
}

#else // QTRA_HAS_WAVESHARE_USBCANB

bool ControlCanDeviceWorker::initChannel(const ChannelConfig &, QString *)
{
    return false;
}

bool ControlCanDeviceWorker::openDevice(const DeviceOpenConfig &config, QString *errorMessage)
{
    QMutexLocker locker(&m_mutex);
    m_config = config;
    m_open = false;
    m_running = false;
    if (errorMessage) {
        *errorMessage = QStringLiteral("Waveshare USBCAN-B lowlevel library is not available. "
                                       "Run tools/add_waveshares_usbcan_submodule.sh or install it to ~/lib.");
    }
    emit statusMessage(QStringLiteral("Waveshare USBCAN-B lowlevel library unavailable"), true);
    emit deviceStateChanged(false);
    return false;
}

void ControlCanDeviceWorker::closeDevice()
{
    QMutexLocker locker(&m_mutex);
    m_running = false;
    m_open = false;
    m_txQueue.clear();
    m_wait.wakeAll();
    emit deviceStateChanged(false);
}

bool ControlCanDeviceWorker::isOpen() const
{
    QMutexLocker locker(&m_mutex);
    return m_open;
}

void ControlCanDeviceWorker::queueTransmit(int, quint32, const QByteArray &, bool, bool)
{
    emit statusMessage(QStringLiteral("Transmit ignored: Waveshare USBCAN-B lowlevel library unavailable"), true);
}

void ControlCanDeviceWorker::clearHardwareBuffers()
{
    emit statusMessage(QStringLiteral("Clear ignored: Waveshare USBCAN-B lowlevel library unavailable"), true);
}

void ControlCanDeviceWorker::resetChannels()
{
    emit statusMessage(QStringLiteral("Reset ignored: Waveshare USBCAN-B lowlevel library unavailable"), true);
}

CanFrame ControlCanDeviceWorker::toFrame(const VCI_CAN_OBJ &obj, int channel, direction_t direction) const
{
    CanFrame frame;
    frame.hostTime = QDateTime::currentDateTime().toString(Qt::ISODateWithMs);
    frame.hwTimestamp = obj.TimeStamp;
    frame.id = obj.ID;
    frame.extended = (obj.ExternFlag != 0);
    frame.remote = (obj.RemoteFlag != 0);
    frame.error = false;
    frame.data = QByteArray(reinterpret_cast<const char *>(obj.Data), obj.DataLen);
    frame.channel = channel;
    frame.direction = direction;
    return frame;
}

void ControlCanDeviceWorker::processPendingTx()
{
}

void ControlCanDeviceWorker::processRxForChannel(const ChannelConfig &)
{
}

void ControlCanDeviceWorker::run()
{
}

#endif // QTRA_HAS_WAVESHARE_USBCANB
