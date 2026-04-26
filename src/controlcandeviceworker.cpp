#include "controlcandeviceworker.h"

#include <QDateTime>
#include <QMutexLocker>
#include <QString>

#include <algorithm>

#if QTRA_HAS_CONTROLCAN

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
    if (result == 1)
        return QStringLiteral("OK");
    if (result == 0)
        return QStringLiteral("operation failed");
    if (result == -1)
        return QStringLiteral("device missing or USB disconnected");
    return QStringLiteral("result=%1").arg(result);
}

bool ControlCanDeviceWorker::initChannel(const ChannelConfig &cfg, QString *errorMessage)
{
    if (!cfg.enabled)
        return true;

    VCI_INIT_CONFIG initCfg{};
    initCfg.AccCode = cfg.accCode;
    initCfg.AccMask = cfg.accMask;
    initCfg.Filter = cfg.filter;
    initCfg.Timing0 = cfg.timing0;
    initCfg.Timing1 = cfg.timing1;
    initCfg.Mode = cfg.mode;

    const long initRes = static_cast<long>(VCI_InitCAN(m_config.deviceType,
                                                       m_config.deviceIndex,
                                                       cfg.canIndex,
                                                       &initCfg));
    if (initRes != 1) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("VCI_InitCAN(ch%1) failed: %2")
                                .arg(cfg.canIndex + 1)
                                .arg(resultToString(initRes));
        }
        return false;
    }

    VCI_ClearBuffer(m_config.deviceType, m_config.deviceIndex, cfg.canIndex);

    const long startRes = static_cast<long>(VCI_StartCAN(m_config.deviceType,
                                                         m_config.deviceIndex,
                                                         cfg.canIndex));
    if (startRes != 1) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("VCI_StartCAN(ch%1) failed: %2")
                                .arg(cfg.canIndex + 1)
                                .arg(resultToString(startRes));
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

    const long openRes = static_cast<long>(VCI_OpenDevice(m_config.deviceType,
                                                          m_config.deviceIndex,
                                                          0));
    if (openRes != 1) {
        if (errorMessage)
            *errorMessage = QStringLiteral("VCI_OpenDevice failed: %1").arg(resultToString(openRes));
        return false;
    }

    QString localError;
    if (!initChannel(m_config.channel0, &localError) || !initChannel(m_config.channel1, &localError)) {
        VCI_CloseDevice(m_config.deviceType, m_config.deviceIndex);
        if (errorMessage)
            *errorMessage = localError;
        return false;
    }

    m_running = true;
    m_open = true;
    locker.unlock();

    start();
    emit statusMessage(QStringLiteral("Device opened"), false);
    emit deviceStateChanged(true);
    return true;
}

void ControlCanDeviceWorker::closeDevice()
{
    {
        QMutexLocker locker(&m_mutex);
        if (!m_open)
            return;
        m_running = false;
        m_wait.wakeAll();
    }

    wait(1000);

    QMutexLocker locker(&m_mutex);
    if (m_config.channel0.enabled)
        VCI_ResetCAN(m_config.deviceType, m_config.deviceIndex, m_config.channel0.canIndex);
    if (m_config.channel1.enabled)
        VCI_ResetCAN(m_config.deviceType, m_config.deviceIndex, m_config.channel1.canIndex);

    VCI_CloseDevice(m_config.deviceType, m_config.deviceIndex);
    m_open = false;
    m_txQueue.clear();
    locker.unlock();

    emit statusMessage(QStringLiteral("Device closed"), false);
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
    tx.data = data;
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
        VCI_ClearBuffer(m_config.deviceType, m_config.deviceIndex, m_config.channel0.canIndex);
    if (m_config.channel1.enabled)
        VCI_ClearBuffer(m_config.deviceType, m_config.deviceIndex, m_config.channel1.canIndex);

    emit statusMessage(QStringLiteral("Hardware buffers cleared"), false);
}

void ControlCanDeviceWorker::resetChannels()
{
    QMutexLocker locker(&m_mutex);
    if (!m_open)
        return;

    if (m_config.channel0.enabled) {
        VCI_ResetCAN(m_config.deviceType, m_config.deviceIndex, m_config.channel0.canIndex);
        VCI_StartCAN(m_config.deviceType, m_config.deviceIndex, m_config.channel0.canIndex);
    }
    if (m_config.channel1.enabled) {
        VCI_ResetCAN(m_config.deviceType, m_config.deviceIndex, m_config.channel1.canIndex);
        VCI_StartCAN(m_config.deviceType, m_config.deviceIndex, m_config.channel1.canIndex);
    }

    emit statusMessage(QStringLiteral("Channels reset"), false);
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
        VCI_CAN_OBJ obj{};
        obj.ID = tx.id;
        obj.SendType = 1;
        obj.RemoteFlag = tx.remote ? 1 : 0;
        obj.ExternFlag = tx.extended ? 1 : 0;
        obj.DataLen = static_cast<BYTE>(std::min(8, tx.data.size()));
        for (int i = 0; i < obj.DataLen; ++i)
            obj.Data[i] = static_cast<BYTE>(tx.data.at(i));

        const long res = static_cast<long>(VCI_Transmit(m_config.deviceType,
                                                        m_config.deviceIndex,
                                                        tx.channel,
                                                        &obj,
                                                        1));
        if (res == 1) {
            if (tx.channel == 0)
                ++m_tx0;
            else
                ++m_tx1;
            emit frameTransmitted(toFrame(obj, tx.channel, dir_tx));
        } else {
            if (tx.channel == 0)
                ++m_err0;
            else
                ++m_err1;
            emit statusMessage(QStringLiteral("VCI_Transmit(ch%1) failed: %2")
                                   .arg(tx.channel + 1)
                                   .arg(resultToString(res)),
                               true);
        }
    }
}

void ControlCanDeviceWorker::processRxForChannel(const ChannelConfig &cfg)
{
    if (!cfg.enabled)
        return;

    const int len = std::max(64, m_config.receiveBatch);
    QVector<VCI_CAN_OBJ> buf(len);
    const long count = static_cast<long>(VCI_Receive(m_config.deviceType,
                                                     m_config.deviceIndex,
                                                     cfg.canIndex,
                                                     buf.data(),
                                                     len,
                                                     0));
    if (count > 0) {
        QVector<CanFrame> frames;
        frames.reserve(static_cast<int>(count));
        for (long i = 0; i < count; ++i)
            frames.push_back(toFrame(buf[static_cast<int>(i)], cfg.canIndex, dir_rx));

        if (cfg.canIndex == 0)
            m_rx0 += static_cast<quint64>(count);
        else
            m_rx1 += static_cast<quint64>(count);
        emit frameBatchReady(frames);
    } else if (count == -1) {
        if (cfg.canIndex == 0)
            ++m_err0;
        else
            ++m_err1;
        emit statusMessage(QStringLiteral("VCI_Receive(ch%1) failed").arg(cfg.canIndex + 1), true);
    }
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

#else

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
    return QStringLiteral("ControlCAN SDK unavailable, result=%1").arg(result);
}

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
        *errorMessage = QStringLiteral("ControlCAN SDK is incomplete. Hardware capture is disabled; use Simulation > Select source for candump/lua replay.");
    }

    emit statusMessage(QStringLiteral("ControlCAN SDK unavailable or incomplete"), true);
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
    emit statusMessage(QStringLiteral("Transmit ignored: ControlCAN SDK unavailable or incomplete"), true);
}

void ControlCanDeviceWorker::clearHardwareBuffers()
{
    emit statusMessage(QStringLiteral("Clear ignored: ControlCAN SDK unavailable or incomplete"), true);
}

void ControlCanDeviceWorker::resetChannels()
{
    emit statusMessage(QStringLiteral("Reset ignored: ControlCAN SDK unavailable or incomplete"), true);
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

#endif
