// SPDX-License-Identifier: GPL-3.0-only
/*
 * QtRNetAnalyzer
 *
 * Copyright (c) 2026
 * ChatGPT (GPT-5.4 Thinking)
 * Jürgen Willi Sievers <JSievers@NadiSoft.de>
 */
#pragma once

#include <QMainWindow>
#include <QMap>
#include <QVector>

#include "canframe.h"

class QLabel;
class QScrollArea;
class QVBoxLayout;
class PlotWidget;

class SignalPlotWindow final : public QMainWindow
{
    Q_OBJECT
public:
    explicit SignalPlotWindow(QWidget *parent = nullptr);

public slots:
    void setTagState(quint64 key, const QString &name, bool tagged);
    void setTaggedHistory(quint64 key, const QString &name, const QVector<CanFrame> &frames);
    void appendTaggedFrame(quint64 key, const QString &name, const CanFrame &frame);
    void clearAll();

private:
    struct PlotEntry
    {
        QString name;
        QWidget *card = nullptr;
        QLabel *title = nullptr;
        PlotWidget *plot = nullptr;
    };

    void ensurePlot(quint64 key, const QString &name);
    void updateEmptyState();

    QWidget *m_root = nullptr;
    QScrollArea *m_scroll = nullptr;
    QWidget *m_container = nullptr;
    QVBoxLayout *m_layout = nullptr;
    QLabel *m_emptyLabel = nullptr;
    QMap<quint64, PlotEntry> m_plots;
};

