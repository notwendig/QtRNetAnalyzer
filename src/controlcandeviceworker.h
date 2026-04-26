#pragma once

#include "canframe.h"

#include <QByteArray>
#include <QMutex>
#include <QThread>
#include <QVector>
#include <QWaitCondition>
#include <QtGlobal>

#ifndef QTRA_HAS_CONTROLCAN
#define QTRA_HAS_CONTROLCAN 0
#endif

#if QTRA_HAS_CONTROLCAN
#include "controlcan.h"
#else
using BYTE = quint8;
using UCHAR = quint8;
using DWORD = quint32;

constexpr DWORD VCI_USBCAN2 = 4;

struct VCI_CAN_OBJ
{
    DWORD ID = 0;
    DWORD TimeStamp = 0;
    BYTE TimeFlag = 0;
    BYTE SendType = 0;
    BYTE RemoteFlag = 0;
    BYTE ExternFlag = 0;
    BYTE DataLen = 0;
    BYTE Data[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    BYTE Reserved[3] = {0, 0, 0};
};
#endif

struct ChannelConfig
{
    bool enabled = true;
    DWORD canIndex = 0;
    DWORD accCode = 0;
    DWORD accMask = 0xFFFFFFFFu;
    UCHAR filter = 1;
    UCHAR timing0 = 0x03;
    UCHAR timing1 = 0x1C;
    UCHAR mode = 1;
};

struct DeviceOpenConfig
{
    DWORD deviceType = VCI_USBCAN2;
    DWORD deviceIndex = 0;
    ChannelConfig channel0;
    ChannelConfig channel1{false, 1, 0, 0xFFFFFFFFu, 1, 0x03, 0x1C, 1};
    int receiveBatch = 2000;
    int pollDelayMs = 10;
};

class ControlCanDeviceWorker final : public QThread
{
    Q_OBJECT

public:
    explicit ControlCanDeviceWorker(QObject *parent = nullptr);
    ~ControlCanDeviceWorker() override;

    bool openDevice(const DeviceOpenConfig &config, QString *errorMessage);
    void closeDevice();
    bool isOpen() const;

public slots:
    void queueTransmit(int channel, quint32 id, const QByteArray &data, bool extended, bool remote);
    void clearHardwareBuffers();
    void resetChannels();

signals:
    void frameBatchReady(const QVector<CanFrame> &frames);
    void frameTransmitted(const CanFrame &frame);
    void countersUpdated(quint64 rx0, quint64 rx1, quint64 tx0, quint64 tx1, quint64 err0, quint64 err1);
    void statusMessage(const QString &message, bool error);
    void deviceStateChanged(bool open);

protected:
    void run() override;

private:
    QString resultToString(long result) const;
    bool initChannel(const ChannelConfig &cfg, QString *errorMessage);
    void processRxForChannel(const ChannelConfig &cfg);
    void processPendingTx();
    CanFrame toFrame(const VCI_CAN_OBJ &obj, int channel, direction_t direction) const;

    mutable QMutex m_mutex;
    QWaitCondition m_wait;
    DeviceOpenConfig m_config;
    bool m_running = false;
    bool m_open = false;
    QVector<CanFrame> m_txQueue;
    quint64 m_rx0 = 0;
    quint64 m_rx1 = 0;
    quint64 m_tx0 = 0;
    quint64 m_tx1 = 0;
    quint64 m_err0 = 0;
    quint64 m_err1 = 0;
};
