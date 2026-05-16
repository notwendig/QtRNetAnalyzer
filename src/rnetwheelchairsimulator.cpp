#include "rnetwheelchairsimulator.h"

#include <QByteArray>
#include <QDateTime>
#include <QString>
#include <QtGlobal>

namespace
{
constexpr int kSimulatedJsmSlot = 3;

// Direction is defined from the ESP/gateway point of view:
//   RX = Android/App -> ESP command; later real-CAN mode sends this CAN frame.
//   TX = ESP -> Android/App report; later real-CAN mode received this CAN frame.
constexpr direction_t kAppToEsp = dir_rx;
constexpr direction_t kEspToApp = dir_tx;

CanFrame makeFrame(quint32 id,
                   const QByteArray &data,
                   quint32 timestampUs,
                   bool extended,
                   bool remote,
                   direction_t direction,
                   int channel = 0)
{
    CanFrame frame;
    frame.id = id;
    frame.data = data;
    frame.extended = extended;
    frame.remote = remote;
    frame.error = false;
    frame.hwTimestamp = timestampUs;
    frame.hostTime = QDateTime::currentDateTime().toString(Qt::ISODateWithMs);
    frame.channel = channel;
    frame.direction = direction;
    return frame;
}

void addStd(QVector<CanFrame> &frames,
            quint32 &timestampUs,
            quint32 id,
            const QByteArray &data = QByteArray(),
            bool remote = false,
            direction_t direction = kAppToEsp,
            quint32 deltaUs = 10000)
{
    frames.push_back(makeFrame(id, data, timestampUs, false, remote, direction));
    timestampUs += deltaUs;
}

void addExt(QVector<CanFrame> &frames,
            quint32 &timestampUs,
            quint32 id,
            const QByteArray &data = QByteArray(),
            direction_t direction = kAppToEsp,
            quint32 deltaUs = 10000)
{
    frames.push_back(makeFrame(id, data, timestampUs, true, false, direction));
    timestampUs += deltaUs;
}

QByteArray bytes(std::initializer_list<unsigned char> values)
{
    QByteArray out;
    out.reserve(static_cast<int>(values.size()));
    for (const unsigned char value : values)
        out.append(static_cast<char>(value));
    return out;
}

quint32 joystickId(int slot)
{
    // Current QtRNetAnalyzer decoder recognizes the 0x02000300 joystick family.
    // Slot 3 is intentionally used for this built-in simulation so the row is decoded
    // as RNetJoystickPosition without changing the existing decoder in this feature pack.
    return 0x02000000u | (static_cast<quint32>(slot & 0x0F) << 8);
}

quint32 deviceFamilyId(quint32 base, int slot)
{
    return base | (static_cast<quint32>(slot & 0x0F) << 8);
}

void addJoystick(QVector<CanFrame> &frames, quint32 &timestampUs, qint8 x, qint8 y)
{
    addExt(frames,
           timestampUs,
           joystickId(kSimulatedJsmSlot),
           bytes({static_cast<unsigned char>(x), static_cast<unsigned char>(y)}),
           kAppToEsp,
           10000);
}

void addRuntimeStatus(QVector<CanFrame> &frames, quint32 &timestampUs, int cycle, qint8 x, qint8 y)
{
    const quint8 absLoad = static_cast<quint8>(qMin(100, qAbs(int(x)) + qAbs(int(y))));
    const quint8 battery = static_cast<quint8>(qMax(20, 96 - (cycle / 35)));

    addExt(frames, timestampUs, 0x03C30F0Fu, bytes({0x52, 0x4E, 0x45, 0x54, 0x53, 0x49, 0x4D, 0x31}), kEspToApp, 10000);
    addExt(frames, timestampUs, deviceFamilyId(0x0C140000u, kSimulatedJsmSlot), bytes({0x01}), kEspToApp, 10000);
    addExt(frames, timestampUs, deviceFamilyId(0x14300000u, kSimulatedJsmSlot), bytes({absLoad, 0x00}), kEspToApp, 10000);
    addExt(frames, timestampUs, deviceFamilyId(0x1C0C0000u, kSimulatedJsmSlot), bytes({battery}), kEspToApp, 10000);
}

void appendSyntheticJsmLogin(QVector<CanFrame> &frames, quint32 &timestampUs)
{
    // Deliberately synthetic/non-authentic R-Net lab sequence:
    // it exercises the QtRNetAnalyzer decoder and UI only. It is not a real JSM
    // authorization sequence and must not be treated as real wheelchair startup data.
    addStd(frames, timestampUs, 0x00Cu, QByteArray(), false, kAppToEsp, 20000); // App -> ESP: RNetJsmCanBusTest
    addStd(frames, timestampUs, 0x7B3u, bytes({0x53, 0x49, 0x4D, 0x2D, 0x41, 0x55, 0x54, 0x48}), false, kAppToEsp, 20000);
    addStd(frames, timestampUs, 0x7B3u, QByteArray(), true, kEspToApp, 20000);
    addStd(frames, timestampUs, 0x00Eu, bytes({0x53, 0x49, 0x4D, 0x4A, 0x53, 0x4D, 0x30, 0x33}), false, kAppToEsp, 20000);

    addStd(frames, timestampUs, 0x780u | kSimulatedJsmSlot, bytes({0x22, 0x10, 0x01, 0x00, 0x03, 0x00, 0x00, 0x00}), false, kAppToEsp, 10000);
    addStd(frames, timestampUs, 0x790u | kSimulatedJsmSlot, bytes({0x60, 0x10, 0x01, 0x00, 0x03, 0x00, 0x00, 0x00}), false, kEspToApp, 10000);
    addStd(frames, timestampUs, 0x040u | kSimulatedJsmSlot, bytes({0x01}), false, kAppToEsp, 10000);
    addStd(frames, timestampUs, 0x040u | kSimulatedJsmSlot, bytes({0x80}), false, kAppToEsp, 10000);

    addStd(frames, timestampUs, 0x051u, bytes({0x01}), false, kAppToEsp, 10000);
    addStd(frames, timestampUs, 0x050u, bytes({0x50, 0x01, 0x00, 0x01}), false, kEspToApp, 10000);
    addStd(frames, timestampUs, 0x061u, bytes({0x00, 0x01}), false, kAppToEsp, 10000);
    addStd(frames, timestampUs, 0x060u, bytes({0x60, 0x01, 0x00, 0x01}), false, kEspToApp, 10000);

    addExt(frames, timestampUs, deviceFamilyId(0x1C240001u, kSimulatedJsmSlot), bytes({0x01}), kEspToApp, 10000); // RNetReady
    addExt(frames, timestampUs, deviceFamilyId(0x0A040000u, kSimulatedJsmSlot), bytes({0x28}), kAppToEsp, 10000); // 40% speed cap
    addJoystick(frames, timestampUs, 0, 0);
}

void appendDriveDemo(QVector<CanFrame> &frames, quint32 &timestampUs)
{
    for (int i = 0; i < 40; ++i)
        addJoystick(frames, timestampUs, 0, 0);

    for (int i = 0; i <= 40; ++i) {
        const qint8 y = static_cast<qint8>(qMin(40, i));
        addJoystick(frames, timestampUs, 0, y);
        if ((i % 10) == 0)
            addRuntimeStatus(frames, timestampUs, i, 0, y);
    }

    for (int i = 0; i < 40; ++i) {
        addJoystick(frames, timestampUs, 0, 40);
        if ((i % 10) == 0)
            addRuntimeStatus(frames, timestampUs, i, 0, 40);
    }

    for (int i = 40; i >= -40; --i) {
        const qint8 x = static_cast<qint8>(i);
        addJoystick(frames, timestampUs, x, 20);
        if ((i % 20) == 0)
            addRuntimeStatus(frames, timestampUs, 40 - i, x, 20);
    }

    for (int i = 40; i >= 0; --i) {
        const qint8 y = static_cast<qint8>(i);
        addJoystick(frames, timestampUs, 0, y);
    }

    for (int i = 0; i < 20; ++i)
        addJoystick(frames, timestampUs, 0, 0);

    addExt(frames, timestampUs, deviceFamilyId(0x1C0C0000u, kSimulatedJsmSlot), bytes({0x5A}), kEspToApp, 10000);
    addExt(frames, timestampUs, deviceFamilyId(0x1C300004u, kSimulatedJsmSlot), bytes({0x10, 0x27, 0x00, 0x00, 0x28, 0x27, 0x00, 0x00}), kEspToApp, 10000);
    addExt(frames, timestampUs, 0x181C0D00u, bytes({0x05, 0x30, 0x05, 0x34, 0x05, 0x37, 0x00, 0x00}), kEspToApp, 10000);
}
} // namespace

QString RNetWheelchairSimulator::scenarioName(Scenario scenario)
{
    switch (scenario) {
    case Scenario::SimulatedJsmLoginAndDrive:
        return QStringLiteral("Simulated R-Net wheelchair + JSM login + drive loop");
    }
    return QStringLiteral("Unknown R-Net wheelchair simulation");
}

QVector<CanFrame> RNetWheelchairSimulator::createScenario(Scenario scenario)
{
    QVector<CanFrame> frames;

    switch (scenario) {
    case Scenario::SimulatedJsmLoginAndDrive: {
        frames.reserve(260);
        quint32 timestampUs = 0;
        appendSyntheticJsmLogin(frames, timestampUs);
        appendDriveDemo(frames, timestampUs);
        break;
    }
    }

    return frames;
}
