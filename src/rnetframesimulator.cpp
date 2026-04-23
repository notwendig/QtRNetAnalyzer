// SPDX-License-Identifier: GPL-3.0-only
/*
 * QtRNetAnalyzer
 *
 * Copyright (c) 2026
 * ChatGPT (GPT-5.4 Thinking)
 * Jürgen Willi Sievers <JSievers@NadiSoft.de>
 */
#include "rnetframesimulator.h"

#include <QDateTime>

namespace {
CanFrame makeFrame(quint32 id, QByteArray data, bool extended = true, bool remote = false)
{
    CanFrame frame;
    frame.id = id;
    frame.data = std::move(data);
    frame.extended = extended;
    frame.remote = remote;
    frame.error = false;
    return frame;
}
}

RNetFrameSimulator::RNetFrameSimulator(QObject *parent)
    : QObject(parent)
{
    m_timer.setInterval(125);
    connect(&m_timer, &QTimer::timeout, this, &RNetFrameSimulator::produceFrame);
}

void RNetFrameSimulator::start()
{
    m_timer.start();
}

void RNetFrameSimulator::stop()
{
    m_timer.stop();
}

void RNetFrameSimulator::produceFrame()
{
    static const QList<CanFrame> patterns = {
        makeFrame(0x000u, {}, false, true),
        makeFrame(0x001u, QByteArray::fromHex("00"), false, false),
        makeFrame(0x00000202u, QByteArray::fromHex("7F80")),
        makeFrame(0x00000300u, QByteArray::fromHex("01")),
        makeFrame(0x00000400u, QByteArray::fromHex("64")),
        makeFrame(0x00000601u, QByteArray::fromHex("0100")),
        makeFrame(0x00000700u, QByteArray::fromHex("01")),
        makeFrame(0x00000D00u, QByteArray::fromHex("58")),
        makeFrame(0x00000E00u, QByteArray::fromHex("0001A2B3")),
        makeFrame(0x00001000u, QByteArray::fromHex("01")),
        makeFrame(0x00001200u, QByteArray::fromHex("1122334455667788")),
        makeFrame(0x12345678u, QByteArray::fromHex("DEADBEEF"))
    };

    CanFrame frame = patterns.at(m_index % patterns.size());
    frame.hwTimestamp = m_timestamp;
    frame.hostTime = QDateTime::currentDateTime().toString(Qt::ISODateWithMs);

    m_timestamp += 20 + static_cast<quint32>((m_index % 5) * 5);
    ++m_index;

    emit frameGenerated(frame);
}
