// SPDX-License-Identifier: GPL-3.0-only
/*
 * QtRNetAnalyzer
 *
 * Copyright (c) 2026
 * ChatGPT (GPT-5.4 Thinking)
 * Jürgen Willi Sievers <JSievers@NadiSoft.de>
 */
#pragma once

#include "canframe.h"

#include <QObject>
#include <QTimer>

class RNetFrameSimulator final : public QObject
{
    Q_OBJECT

public:
    explicit RNetFrameSimulator(QObject *parent = nullptr);

public slots:
    void start();
    void stop();

signals:
    void frameGenerated(const CanFrame &frame);

private slots:
    void produceFrame();

private:
    quint32 m_timestamp = 0;
    int m_index = 0;
    QTimer m_timer;
};
