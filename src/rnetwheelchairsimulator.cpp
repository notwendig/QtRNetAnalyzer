#include "rnetwheelchairsimulator.h"

#include <QByteArray>
#include <QDateTime>

namespace
{
QByteArray fromHex(const char *hex)
{
    return QByteArray::fromHex(QByteArray(hex));
}
}

CanFrame RNetWheelchairSimulator::makeFrame(quint32 timestampUs,
                                            quint32 id,
                                            const QByteArray &data,
                                            direction_t direction,
                                            int channel,
                                            bool extended)
{
    CanFrame frame;
    frame.id = id;
    frame.data = data.left(8);
    frame.extended = extended;
    frame.remote = false;
    frame.error = false;
    frame.hwTimestamp = timestampUs;
    frame.hostTime = QDateTime::currentDateTime().toString(Qt::ISODateWithMs);
    frame.channel = channel;
    frame.direction = direction;
    return frame;
}

QVector<CanFrame> RNetWheelchairSimulator::createScenario(Scenario scenario)
{
    Q_UNUSED(scenario);

    QVector<CanFrame> frames;
    frames.reserve(48);

    quint32 t = 0;
    auto add = [&](quint32 id, const char *hex, direction_t direction, int channel = 0, quint32 stepUs = 10000) {
        frames.push_back(makeFrame(t, id, fromHex(hex), direction, channel, true));
        t += stepUs;
    };

    // Synthetic, non-authentic lab sequence. It is intended only to exercise
    // the analyzer tables, aggregation, direction display and signal plotting.
    add(0x02000300u, "0000000000000000", dir_rx, 0, 8000);
    add(0x02000301u, "0100000000000000", dir_tx, 0, 8000);
    add(0x0C000100u, "1000000000000000", dir_rx, 0, 12000);
    add(0x0C000101u, "1100000000000000", dir_tx, 0, 12000);

    for (int i = 0; i < 20; ++i) {
        const quint8 speed = static_cast<quint8>((i < 10) ? (i * 8) : ((19 - i) * 8));
        const quint8 steer = static_cast<quint8>((i % 7) * 4);
        QByteArray joystick;
        joystick.append(char(speed));
        joystick.append(char(steer));
        joystick.append(char(0x00));
        joystick.append(char(0x00));
        joystick.append(char(0x00));
        joystick.append(char(0x00));
        joystick.append(char(0x00));
        joystick.append(char(0x00));
        frames.push_back(makeFrame(t, 0x1C0C0000u, joystick, dir_rx, 0, true));
        t += 10000;

        QByteArray status;
        status.append(char(0x40));
        status.append(char(speed));
        status.append(char(0x20));
        status.append(char(0x00));
        status.append(char(0x00));
        status.append(char(0x00));
        status.append(char(0x00));
        status.append(char(0x00));
        frames.push_back(makeFrame(t, 0x1C0C0100u, status, dir_tx, 0, true));
        t += 10000;
    }

    add(0x0C000102u, "1200000000000000", dir_rx, 0, 12000);
    add(0x02000302u, "0200000000000000", dir_tx, 0, 8000);

    return frames;
}

QString RNetWheelchairSimulator::scenarioName(Scenario scenario)
{
    Q_UNUSED(scenario);
    return QStringLiteral("Built-in synthetic R-Net wheelchair simulation (JSM login lab sequence)");
}
