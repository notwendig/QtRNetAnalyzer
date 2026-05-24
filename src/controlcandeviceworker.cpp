#include "controlcandeviceworker.h"

#include <QDateTime>
#include <QMutexLocker>
#include <QAction>
#include <QActionGroup>
#include <QMenu>
#include <QMenuBar>
#include <QMainWindow>
#include <QVariant>

#include <QCanBus>
#include <QCanBusDeviceInfo>

#include <algorithm>
#include <cerrno>
#include <cstring>

#if QTRA_HAS_SOCKETCAN
#include <fcntl.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace {

constexpr int kChannelCount = 2;

#if QTRA_HAS_SOCKETCAN
void closeFd(int &fd)
{
    if (fd >= 0) {
        ::close(fd);
        fd = -1;
    }
}
#endif

} // namespace

ControlCanDeviceWorker::ControlCanDeviceWorker(QObject *parent)
    : QThread(parent)
{
    installDeviceMenu(parent);
}

ControlCanDeviceWorker::~ControlCanDeviceWorker()
{
    closeDevice();
}


void ControlCanDeviceWorker::installDeviceMenu(QObject *parentObject)
{
    auto *window = qobject_cast<QMainWindow *>(parentObject);
    if (!window) {
        return;
    }

    m_deviceMenu = window->menuBar()->addMenu(QStringLiteral("&Device"));
    refreshDeviceMenu();
}

QStringList ControlCanDeviceWorker::availableSocketCanInterfaces(QString *errorMessage) const
{
    QStringList interfaces;

#if !QTRA_HAS_SOCKETCAN
    if (errorMessage) {
        *errorMessage = QStringLiteral("SocketCAN backend is disabled in this build.");
    }
    return interfaces;
#else
    QCanBus *bus = QCanBus::instance();
    if (!bus) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("QCanBus singleton is unavailable.");
        }
        return interfaces;
    }

    const QStringList plugins = bus->plugins();
    if (!plugins.contains(QStringLiteral("socketcan"))) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Qt SocketCAN plugin not found. Install Qt6 SerialBus/socketcan plugin support.");
        }
        return interfaces;
    }

    QString qtError;
    const QList<QCanBusDeviceInfo> devices = bus->availableDevices(QStringLiteral("socketcan"), &qtError);
    for (const QCanBusDeviceInfo &device : devices) {
        const QString name = device.name().trimmed();
        if (!name.isEmpty() && !interfaces.contains(name)) {
            interfaces.push_back(name);
        }
    }

    interfaces.sort(Qt::CaseInsensitive);

    if (interfaces.isEmpty() && !qtError.isEmpty()) {
        if (errorMessage) {
            *errorMessage = qtError;
        }
    }

    return interfaces;
#endif
}

QString ControlCanDeviceWorker::deviceSelectionText(const QStringList &interfaces) const
{
    if (interfaces.isEmpty()) {
        return QStringLiteral("Auto by device index pair (can0/can1, can2/can3, ...)");
    }

    if (interfaces.size() == 1) {
        return QStringLiteral("CAN1=%1, CAN2 disabled").arg(interfaces.at(0));
    }

    return QStringLiteral("CAN1=%1, CAN2=%2").arg(interfaces.at(0), interfaces.at(1));
}

void ControlCanDeviceWorker::addDeviceSelectionAction(QMenu *menu,
                                                      QActionGroup *group,
                                                      const QString &label,
                                                      const QStringList &interfaces,
                                                      bool checked)
{
    if (!menu || !group) {
        return;
    }

    QAction *action = menu->addAction(label);
    action->setCheckable(true);
    action->setChecked(checked);
    action->setData(QVariant::fromValue(interfaces));
    group->addAction(action);
    connect(action, &QAction::triggered, this, &ControlCanDeviceWorker::selectDeviceInterfaces);
}

void ControlCanDeviceWorker::refreshDeviceMenu()
{
    if (!m_deviceMenu) {
        return;
    }

    if (m_deviceActionGroup) {
        delete m_deviceActionGroup;
        m_deviceActionGroup = nullptr;
    }

    m_deviceMenu->clear();

    QAction *refreshAction = m_deviceMenu->addAction(QStringLiteral("Refresh SocketCAN devices"));
    connect(refreshAction, &QAction::triggered, this, &ControlCanDeviceWorker::refreshDeviceMenu);
    m_deviceMenu->addSeparator();

    m_deviceActionGroup = new QActionGroup(m_deviceMenu);
    m_deviceActionGroup->setExclusive(true);

    addDeviceSelectionAction(m_deviceMenu,
                             m_deviceActionGroup,
                             QStringLiteral("Auto: device index pair (can0/can1, can2/can3, ...)"),
                             QStringList{},
                             m_selectedInterfaces.isEmpty());

    QString error;
    const QStringList interfaces = availableSocketCanInterfaces(&error);

    if (!interfaces.isEmpty()) {
        m_deviceMenu->addSeparator();
        m_deviceMenu->addSection(QStringLiteral("Detected SocketCAN devices"));

        QStringList addedPairKeys;
        for (const QString &name : interfaces) {
            if (!name.startsWith(QStringLiteral("can"))) {
                continue;
            }

            bool ok = false;
            const int number = name.mid(3).toInt(&ok);
            if (!ok || (number % 2) != 0) {
                continue;
            }

            const QString second = QStringLiteral("can%1").arg(number + 1);
            if (!interfaces.contains(second)) {
                continue;
            }

            const QStringList pair{name, second};
            const QString key = pair.join(QLatin1Char('|'));
            if (addedPairKeys.contains(key)) {
                continue;
            }
            addedPairKeys.push_back(key);

            addDeviceSelectionAction(m_deviceMenu,
                                     m_deviceActionGroup,
                                     QStringLiteral("waveUSBCAN_b pair: CAN1=%1, CAN2=%2").arg(name, second),
                                     pair,
                                     m_selectedInterfaces == pair);
        }

        if (addedPairKeys.isEmpty() && interfaces.size() >= 2) {
            const QStringList pair{interfaces.at(0), interfaces.at(1)};
            addDeviceSelectionAction(m_deviceMenu,
                                     m_deviceActionGroup,
                                     QStringLiteral("Pair: CAN1=%1, CAN2=%2").arg(pair.at(0), pair.at(1)),
                                     pair,
                                     m_selectedInterfaces == pair);
        }

        m_deviceMenu->addSeparator();
        for (const QString &name : interfaces) {
            const QStringList single{name};
            addDeviceSelectionAction(m_deviceMenu,
                                     m_deviceActionGroup,
                                     QStringLiteral("Single channel: CAN1=%1").arg(name),
                                     single,
                                     m_selectedInterfaces == single);
        }
    } else {
        QAction *emptyAction = m_deviceMenu->addAction(
            error.isEmpty()
                ? QStringLiteral("No SocketCAN devices detected")
                : QStringLiteral("No SocketCAN devices detected: %1").arg(error));
        emptyAction->setEnabled(false);
    }

    m_deviceMenu->addSeparator();
    QAction *hintAction = m_deviceMenu->addAction(QStringLiteral("Hint: install/run waveUSBCAN_b, then refresh"));
    hintAction->setEnabled(false);
}

void ControlCanDeviceWorker::selectDeviceInterfaces()
{
    auto *action = qobject_cast<QAction *>(sender());
    if (!action) {
        return;
    }

    const QStringList interfaces = action->data().toStringList();
    {
        QMutexLocker locker(&m_mutex);
        m_selectedInterfaces = interfaces;
    }

    emit statusMessage(QStringLiteral("Device selection: %1. Close and open capture to apply if already running.")
                           .arg(deviceSelectionText(interfaces)),
                       false);
}

QString ControlCanDeviceWorker::resultToString(long result) const
{
#if QTRA_HAS_SOCKETCAN
    if (result < 0) {
        return QString::fromLocal8Bit(std::strerror(static_cast<int>(-result)));
    }
#endif
    return QStringLiteral("result=%1").arg(result);
}

QString ControlCanDeviceWorker::interfaceNameForChannel(const ChannelConfig &cfg) const
{
    const int channelIndex = static_cast<int>(cfg.canIndex);
    if (channelIndex >= 0 && channelIndex < m_activeInterfaces.size()) {
        const QString selected = m_activeInterfaces.at(channelIndex).trimmed();
        if (!selected.isEmpty()) {
            return selected;
        }
    }

    const int base = static_cast<int>(m_config.deviceIndex) * kChannelCount;
    return QStringLiteral("can%1").arg(base + channelIndex);
}

int ControlCanDeviceWorker::bitrateFromTiming(UCHAR timing0, UCHAR timing1) const
{
    struct Entry {
        int bitrate;
        UCHAR timing0;
        UCHAR timing1;
    };

    static constexpr Entry entries[] = {
        {1000000, 0x00, 0x14},
        {500000,  0x00, 0x1C},
        {250000,  0x01, 0x1C},
        {125000,  0x03, 0x1C},
        {100000,  0x04, 0x1C},
        {50000,   0x09, 0x1C},
    };

    for (const Entry &entry : entries) {
        if (entry.timing0 == timing0 && entry.timing1 == timing1) {
            return entry.bitrate;
        }
    }

    return 0;
}

bool ControlCanDeviceWorker::initChannel(const ChannelConfig &cfg, QString *errorMessage)
{
    if (!cfg.enabled) {
        if (cfg.canIndex < m_channels.size()) {
            m_channels[static_cast<std::size_t>(cfg.canIndex)] = RuntimeChannel{};
        }
        return true;
    }

    if (cfg.canIndex >= m_channels.size()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Invalid SocketCAN channel index %1")
                                .arg(static_cast<unsigned>(cfg.canIndex));
        }
        return false;
    }

#if !QTRA_HAS_SOCKETCAN
    Q_UNUSED(cfg);
    if (errorMessage) {
        *errorMessage = QStringLiteral("SocketCAN backend is not available in this build. "
                                       "Build on Linux with QTRNET_ENABLE_SOCKETCAN=ON.");
    }
    return false;
#else
    const QString ifName = interfaceNameForChannel(cfg);
    const QByteArray ifName8 = ifName.toLocal8Bit();

    int fd = ::socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (fd < 0) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("socket(PF_CAN) failed for %1: %2")
                                .arg(ifName, QString::fromLocal8Bit(std::strerror(errno)));
        }
        return false;
    }

    if (::fcntl(fd, F_SETFL, O_NONBLOCK) < 0) {
        const int savedErrno = errno;
        ::close(fd);
        if (errorMessage) {
            *errorMessage = QStringLiteral("fcntl(O_NONBLOCK) failed for %1: %2")
                                .arg(ifName, QString::fromLocal8Bit(std::strerror(savedErrno)));
        }
        return false;
    }

    ifreq ifr{};
    std::strncpy(ifr.ifr_name, ifName8.constData(), IFNAMSIZ - 1);

    if (::ioctl(fd, SIOCGIFINDEX, &ifr) < 0) {
        const int savedErrno = errno;
        ::close(fd);
        if (errorMessage) {
            *errorMessage = QStringLiteral("SocketCAN interface %1 not found: %2. "
                                           "Load/install waveUSBCAN_b and check `ip link show type can`.")
                                .arg(ifName, QString::fromLocal8Bit(std::strerror(savedErrno)));
        }
        return false;
    }

    ifreq flagsReq{};
    std::strncpy(flagsReq.ifr_name, ifName8.constData(), IFNAMSIZ - 1);
    if (::ioctl(fd, SIOCGIFFLAGS, &flagsReq) == 0) {
        if ((flagsReq.ifr_flags & IFF_UP) == 0) {
            ::close(fd);
            const int requestedBitrate = bitrateFromTiming(cfg.timing0, cfg.timing1);
            if (errorMessage) {
                if (requestedBitrate > 0) {
                    *errorMessage = QStringLiteral(
                        "%1 is down. Bring it up first, for example: "
                        "sudo ip link set %1 type can bitrate %2 restart-ms 100 && sudo ip link set %1 up")
                                        .arg(ifName)
                                        .arg(requestedBitrate);
                } else {
                    *errorMessage = QStringLiteral("%1 is down. Bring the SocketCAN interface up first.")
                                        .arg(ifName);
                }
            }
            return false;
        }
    }

    int recvOwnMessages = 0;
    ::setsockopt(fd, SOL_CAN_RAW, CAN_RAW_RECV_OWN_MSGS, &recvOwnMessages, sizeof(recvOwnMessages));

    sockaddr_can addr{};
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    if (::bind(fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0) {
        const int savedErrno = errno;
        ::close(fd);
        if (errorMessage) {
            *errorMessage = QStringLiteral("bind(%1) failed: %2")
                                .arg(ifName, QString::fromLocal8Bit(std::strerror(savedErrno)));
        }
        return false;
    }

    RuntimeChannel runtime;
    runtime.fd = fd;
    runtime.channel = static_cast<int>(cfg.canIndex);
    runtime.interfaceName = ifName;
    runtime.enabled = true;
    m_channels[static_cast<std::size_t>(cfg.canIndex)] = runtime;

    return true;
#endif
}

bool ControlCanDeviceWorker::openDevice(const DeviceOpenConfig &config, QString *errorMessage)
{
    {
        QMutexLocker locker(&m_mutex);
        if (m_open) {
            if (errorMessage) {
                *errorMessage = QStringLiteral("Device already open");
            }
            return false;
        }

        m_config = config;
        m_activeInterfaces = m_selectedInterfaces;
        if (!m_activeInterfaces.isEmpty()) {
            m_config.deviceIndex = 0;
            m_config.channel0.canIndex = 0;
            if (m_activeInterfaces.at(0).trimmed().isEmpty()) {
                m_config.channel0.enabled = false;
            }
            m_config.channel1.canIndex = 1;
            if (m_activeInterfaces.size() < 2 || m_activeInterfaces.at(1).trimmed().isEmpty()) {
                m_config.channel1.enabled = false;
            }
        }
        m_rx0 = m_rx1 = m_tx0 = m_tx1 = m_err0 = m_err1 = 0;
        m_txQueue.clear();
        m_channels = {};
    }

    QString localError;
    if (!initChannel(m_config.channel0, &localError) || !initChannel(m_config.channel1, &localError)) {
        closeAllSockets();
        if (errorMessage) {
            *errorMessage = localError;
        }
        return false;
    }

    const bool anyChannelOpen = std::any_of(m_channels.cbegin(), m_channels.cend(), [](const RuntimeChannel &channel) {
        return channel.enabled && channel.fd >= 0;
    });

    if (!anyChannelOpen) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("No SocketCAN channel is enabled");
        }
        return false;
    }

    {
        QMutexLocker locker(&m_mutex);
        m_running = true;
        m_open = true;
    }

    start();

    emit statusMessage(QStringLiteral("SocketCAN capture opened: %1").arg(deviceSelectionText(m_activeInterfaces)), false);
    emit statusMessage(QStringLiteral("Bitrate and listen-only mode are configured by Linux `ip link`/waveUSBCAN_b, not by the Qt UI."), false);
    emit deviceStateChanged(true);
    return true;
}

void ControlCanDeviceWorker::closeDevice()
{
    bool shouldClose = false;
    {
        QMutexLocker locker(&m_mutex);
        shouldClose = m_open || m_running;
        m_running = false;
        m_wait.wakeAll();
    }

    if (!shouldClose) {
        return;
    }

    if (isRunning()) {
        if (!wait(3000)) {
            requestInterruption();
            wait(1000);
        }
    }

    closeAllSockets();

    {
        QMutexLocker locker(&m_mutex);
        m_open = false;
        m_activeInterfaces.clear();
        m_txQueue.clear();
    }

    emit statusMessage(QStringLiteral("SocketCAN capture closed"), false);
    emit deviceStateChanged(false);
}

bool ControlCanDeviceWorker::isOpen() const
{
    QMutexLocker locker(&m_mutex);
    return m_open;
}

void ControlCanDeviceWorker::queueTransmit(int channel, quint32 id, const QByteArray &data, bool extended, bool remote)
{
    bool accepted = false;
    QString error;

    {
        QMutexLocker locker(&m_mutex);
        if (!m_open) {
            error = QStringLiteral("Transmit ignored: SocketCAN device is not open");
        } else if (channel < 0 || channel >= static_cast<int>(m_channels.size()) || m_channels[static_cast<std::size_t>(channel)].fd < 0) {
            error = QStringLiteral("Transmit ignored: CAN%1 is not open").arg(channel + 1);
        } else {
            CanFrame tx;
            tx.channel = channel;
            tx.id = id;
            tx.data = data.left(8);
            tx.extended = extended;
            tx.remote = remote;
            tx.direction = dir_tx;
            m_txQueue.push_back(tx);
            accepted = true;
            m_wait.wakeAll();
        }
    }

    if (!accepted) {
        emit statusMessage(error, true);
    }
}

void ControlCanDeviceWorker::clearHardwareBuffers()
{
#if !QTRA_HAS_SOCKETCAN
    emit statusMessage(QStringLiteral("Clear ignored: SocketCAN backend unavailable"), true);
#else
    int drained = 0;

    for (const RuntimeChannel &channel : m_channels) {
        if (channel.fd < 0) {
            continue;
        }

        while (true) {
            can_frame socketFrame{};
            const ssize_t n = ::read(channel.fd, &socketFrame, sizeof(socketFrame));
            if (n == static_cast<ssize_t>(sizeof(socketFrame))) {
                ++drained;
                continue;
            }
            if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                break;
            }
            break;
        }
    }

    emit statusMessage(QStringLiteral("SocketCAN receive queues drained (%1 frame(s))").arg(drained), false);
#endif
}

void ControlCanDeviceWorker::resetChannels()
{
    emit statusMessage(QStringLiteral("SocketCAN reset is handled outside QtRNetAnalyzer with `ip link set canX down/up` or waveUSBCAN_b auto-up."), false);
}

#if QTRA_HAS_SOCKETCAN
CanFrame ControlCanDeviceWorker::toFrame(const can_frame &socketFrame, int channel, direction_t direction) const
{
    CanFrame frame;
    frame.hostTime = QDateTime::currentDateTime().toString(Qt::ISODateWithMs);
    frame.hwTimestamp = 0;
    frame.extended = (socketFrame.can_id & CAN_EFF_FLAG) != 0;
    frame.remote = (socketFrame.can_id & CAN_RTR_FLAG) != 0;
    frame.error = (socketFrame.can_id & CAN_ERR_FLAG) != 0;
    frame.id = socketFrame.can_id & (frame.extended ? CAN_EFF_MASK : CAN_SFF_MASK);
    frame.channel = channel;
    frame.direction = direction;

    if (!frame.remote) {
        const int len = std::min<int>(CAN_MAX_DLEN, socketFrame.len);
        frame.data = QByteArray(reinterpret_cast<const char *>(socketFrame.data), len);
    }

    return frame;
}
#endif

void ControlCanDeviceWorker::processPendingTx()
{
#if !QTRA_HAS_SOCKETCAN
    return;
#else
    QVector<CanFrame> queue;
    {
        QMutexLocker locker(&m_mutex);
        queue = std::move(m_txQueue);
        m_txQueue.clear();
    }

    for (const CanFrame &tx : queue) {
        if (tx.channel < 0 || tx.channel >= static_cast<int>(m_channels.size())) {
            continue;
        }

        const RuntimeChannel &channel = m_channels[static_cast<std::size_t>(tx.channel)];
        if (channel.fd < 0) {
            if (tx.channel == 0) {
                ++m_err0;
            } else {
                ++m_err1;
            }
            emit statusMessage(QStringLiteral("Transmit failed: CAN%1 is not open").arg(tx.channel + 1), true);
            continue;
        }

        can_frame socketFrame{};
        socketFrame.can_id = tx.id & (tx.extended ? CAN_EFF_MASK : CAN_SFF_MASK);
        if (tx.extended) {
            socketFrame.can_id |= CAN_EFF_FLAG;
        }
        if (tx.remote) {
            socketFrame.can_id |= CAN_RTR_FLAG;
        }

        socketFrame.len = static_cast<__u8>(std::min<int>(CAN_MAX_DLEN, tx.data.size()));
        for (int i = 0; i < socketFrame.len; ++i) {
            socketFrame.data[i] = static_cast<__u8>(tx.data.at(i));
        }

        const ssize_t n = ::write(channel.fd, &socketFrame, sizeof(socketFrame));
        if (n == static_cast<ssize_t>(sizeof(socketFrame))) {
            if (tx.channel == 0) {
                ++m_tx0;
            } else {
                ++m_tx1;
            }
            emit frameTransmitted(toFrame(socketFrame, tx.channel, dir_tx));
        } else {
            const int savedErrno = errno;
            if (tx.channel == 0) {
                ++m_err0;
            } else {
                ++m_err1;
            }
            emit statusMessage(QStringLiteral("SocketCAN transmit on %1 failed: %2")
                                   .arg(channel.interfaceName, QString::fromLocal8Bit(std::strerror(savedErrno))),
                               true);
        }
    }
#endif
}

void ControlCanDeviceWorker::processRxForChannel(const ChannelConfig &cfg)
{
#if !QTRA_HAS_SOCKETCAN
    Q_UNUSED(cfg);
    return;
#else
    if (!cfg.enabled || cfg.canIndex >= m_channels.size()) {
        return;
    }

    const RuntimeChannel &channel = m_channels[static_cast<std::size_t>(cfg.canIndex)];
    if (channel.fd < 0) {
        return;
    }

    QVector<CanFrame> frames;
    frames.reserve(std::min(256, std::max(1, m_config.receiveBatch)));

    const int limit = std::max(1, m_config.receiveBatch);
    for (int i = 0; i < limit; ++i) {
        can_frame socketFrame{};
        const ssize_t n = ::read(channel.fd, &socketFrame, sizeof(socketFrame));
        if (n == static_cast<ssize_t>(sizeof(socketFrame))) {
            frames.push_back(toFrame(socketFrame, channel.channel, dir_rx));
            continue;
        }

        if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            break;
        }

        if (n < 0) {
            if (channel.channel == 0) {
                ++m_err0;
            } else {
                ++m_err1;
            }
            emit statusMessage(QStringLiteral("SocketCAN receive on %1 failed: %2")
                                   .arg(channel.interfaceName, QString::fromLocal8Bit(std::strerror(errno))),
                               true);
        }
        break;
    }

    if (!frames.isEmpty()) {
        if (channel.channel == 0) {
            m_rx0 += static_cast<quint64>(frames.size());
        } else {
            m_rx1 += static_cast<quint64>(frames.size());
        }
        emit frameBatchReady(frames);
    }
#endif
}

void ControlCanDeviceWorker::run()
{
#if !QTRA_HAS_SOCKETCAN
    return;
#else
    while (!isInterruptionRequested()) {
        {
            QMutexLocker locker(&m_mutex);
            if (!m_running) {
                break;
            }
        }

        processPendingTx();

        std::array<pollfd, kChannelCount> pollFds{};
        std::array<int, kChannelCount> pollChannels{};
        int pollCount = 0;

        for (const RuntimeChannel &channel : m_channels) {
            if (channel.fd < 0) {
                continue;
            }
            pollFds[static_cast<std::size_t>(pollCount)].fd = channel.fd;
            pollFds[static_cast<std::size_t>(pollCount)].events = POLLIN;
            pollChannels[static_cast<std::size_t>(pollCount)] = channel.channel;
            ++pollCount;
        }

        const int timeoutMs = std::max(1, m_config.pollDelayMs);
        const int pollResult = pollCount > 0 ? ::poll(pollFds.data(), static_cast<nfds_t>(pollCount), timeoutMs) : 0;

        if (pollResult > 0) {
            for (int i = 0; i < pollCount; ++i) {
                if ((pollFds[static_cast<std::size_t>(i)].revents & POLLIN) == 0) {
                    continue;
                }

                const int channel = pollChannels[static_cast<std::size_t>(i)];
                if (channel == 0) {
                    processRxForChannel(m_config.channel0);
                } else if (channel == 1) {
                    processRxForChannel(m_config.channel1);
                }
            }
        } else if (pollResult < 0 && errno != EINTR) {
            emit statusMessage(QStringLiteral("SocketCAN poll failed: %1").arg(QString::fromLocal8Bit(std::strerror(errno))), true);
        }

        emit countersUpdated(m_rx0, m_rx1, m_tx0, m_tx1, m_err0, m_err1);
    }
#endif
}

void ControlCanDeviceWorker::closeAllSockets()
{
#if QTRA_HAS_SOCKETCAN
    for (RuntimeChannel &channel : m_channels) {
        closeFd(channel.fd);
        channel.enabled = false;
    }
#endif
}
