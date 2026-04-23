// SPDX-License-Identifier: GPL-3.0-only
/*
 * QtRNetAnalyzer
 *
 * Copyright (c) 2026
 * ChatGPT (GPT-5.4 Thinking)
 * Jürgen Willi Sievers <JSievers@NadiSoft.de>
 */
#include "signalplotwindow.h"

#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QWidget>
#include <algorithm>
#include <utility>

struct Sample
{
    double x = 0.0;
    QVector<double> values;
};

class PlotWidget final : public QWidget
{
public:
    explicit PlotWidget(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        setMinimumHeight(240);
    }

    void setFrames(const QVector<CanFrame> &frames)
    {
        m_samples.clear();
        m_samples.reserve(frames.size());
        for (const CanFrame &frame : frames)
            m_samples.push_back(makeSample(frame));
        trimIfNeeded();
        update();
    }

    void append(const CanFrame &frame)
    {
        m_samples.push_back(makeSample(frame));
        trimIfNeeded();
        update();
    }

    int sampleCount() const
    {
        return m_samples.size();
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.fillRect(rect(), QColor("#101318"));

        const QRectF plotRect = rect().adjusted(56, 18, -16, -34);
        p.setPen(QColor("#2a3442"));
        for (int i = 0; i <= 4; ++i)
        {
            const qreal y = plotRect.top() + (plotRect.height() * i / 4.0);
            p.drawLine(QPointF(plotRect.left(), y), QPointF(plotRect.right(), y));
        }
        p.drawRect(plotRect);

        p.setPen(QColor("#9aa7b5"));
        for (int i = 0; i <= 4; ++i)
        {
            const int value = 255 - (255 * i / 4);
            const qreal y = plotRect.top() + (plotRect.height() * i / 4.0);
            p.drawText(QRectF(4, y - 10, 46, 20), Qt::AlignRight | Qt::AlignVCenter, QString::number(value));
        }

        if (m_samples.size() < 2)
        {
            p.setPen(QColor("#c9d1d9"));
            p.drawText(plotRect, Qt::AlignCenter, QStringLiteral("Noch keine Kurve"));
            return;
        }

        const double minX = m_samples.front().x;
        const double maxX = m_samples.back().x;
        const double spanX = std::max(1.0, maxX - minX);

        const QVector<QColor> colors = {
            QColor("#ff6b6b"), QColor("#4dabf7"), QColor("#51cf66"), QColor("#ffd43b"),
            QColor("#b197fc"), QColor("#ffa94d"), QColor("#63e6be"), QColor("#f783ac")
        };

        auto toPoint = [&](double x, double y) {
            const qreal px = plotRect.left() + ((x - minX) / spanX) * plotRect.width();
            const qreal py = plotRect.bottom() - (y / 255.0) * plotRect.height();
            return QPointF(px, py);
        };

        for (int signal = 0; signal < 8; ++signal)
        {
            QPainterPath path;
            bool first = true;
            for (const Sample &s : std::as_const(m_samples))
            {
                const QPointF pt = toPoint(s.x, s.values.value(signal));
                if (first)
                {
                    path.moveTo(pt);
                    first = false;
                }
                else
                {
                    path.lineTo(pt);
                }
            }
            p.setPen(QPen(colors.at(signal), 1.8));
            p.drawPath(path);
        }

        p.setPen(QColor("#7f8c98"));
        p.drawText(QRectF(plotRect.left(), plotRect.bottom() + 4, plotRect.width(), 16),
                   Qt::AlignLeft | Qt::AlignVCenter,
                   QStringLiteral("History: %1 Samples").arg(m_samples.size()));

        QRectF legendRect(plotRect.left(), rect().bottom() - 22, plotRect.width(), 18);
        qreal x = legendRect.left();
        for (int signal = 0; signal < 8; ++signal)
        {
            p.setPen(Qt::NoPen);
            p.setBrush(colors.at(signal));
            p.drawRoundedRect(QRectF(x, legendRect.top() + 3, 12, 12), 3, 3);
            p.setPen(QColor("#d0d7de"));
            p.drawText(QRectF(x + 16, legendRect.top(), 44, 18), Qt::AlignLeft | Qt::AlignVCenter,
                       QStringLiteral("B%1").arg(signal));
            x += 52;
        }
    }

private:
    static Sample makeSample(const CanFrame &frame)
    {
        Sample s;
        s.x = static_cast<double>(frame.hwTimestamp);
        s.values.resize(8);
        for (int i = 0; i < 8; ++i)
            s.values[i] = (i < frame.data.size()) ? static_cast<quint8>(frame.data.at(i)) : 0.0;
        return s;
    }

    void trimIfNeeded()
    {
        constexpr int kMaxSamples = 4000;
        if (m_samples.size() > kMaxSamples)
            m_samples.erase(m_samples.begin(), m_samples.end() - kMaxSamples);
    }

    QVector<Sample> m_samples;
};

SignalPlotWindow::SignalPlotWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowFlag(Qt::Window, true);
    setAttribute(Qt::WA_DeleteOnClose, false);
    setWindowTitle(QStringLiteral("R-Net Live-Kurven"));
    resize(1100, 760);

    m_root = new QWidget(this);
    auto *rootLayout = new QVBoxLayout(m_root);
    rootLayout->setContentsMargins(8, 8, 8, 8);

    auto *info = new QLabel(
        QStringLiteral("Zeigt für jede getaggte R-Net-Botschaft alle 8 Datenbytes als Live-Kurven (B0..B7), inklusive vorhandener History."), m_root);
    info->setWordWrap(true);
    rootLayout->addWidget(info);

    m_scroll = new QScrollArea(m_root);
    m_scroll->setWidgetResizable(true);
    m_container = new QWidget(m_scroll);
    m_layout = new QVBoxLayout(m_container);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(10);
    m_emptyLabel = new QLabel(QStringLiteral("Noch keine Tags aktiv."), m_container);
    m_emptyLabel->setAlignment(Qt::AlignCenter);
    m_emptyLabel->setMinimumHeight(240);
    m_layout->addWidget(m_emptyLabel);
    m_layout->addStretch(1);
    m_scroll->setWidget(m_container);

    rootLayout->addWidget(m_scroll, 1);
    setCentralWidget(m_root);
}

void SignalPlotWindow::ensurePlot(quint64 key, const QString &name)
{
    if (m_plots.contains(key))
    {
        m_plots[key].name = name;
        if (m_plots[key].title)
            m_plots[key].title->setText(QStringLiteral("%1  [0x%2]").arg(name).arg(key, 0, 16));
        return;
    }

    PlotEntry entry;
    entry.name = name;
    entry.card = new QWidget(m_container);
    auto *cardLayout = new QVBoxLayout(entry.card);
    cardLayout->setContentsMargins(10, 10, 10, 10);
    cardLayout->setSpacing(6);
    entry.title = new QLabel(QStringLiteral("%1  [0x%2]").arg(name).arg(key, 0, 16), entry.card);
    entry.title->setStyleSheet(QStringLiteral("font-weight:700;"));
    entry.plot = new PlotWidget(entry.card);
    cardLayout->addWidget(entry.title);
    cardLayout->addWidget(entry.plot);
    m_layout->insertWidget(std::max(0, m_layout->count() - 1), entry.card);
    m_plots.insert(key, entry);
    updateEmptyState();
}

void SignalPlotWindow::setTagState(quint64 key, const QString &name, bool tagged)
{
    if (tagged)
    {
        ensurePlot(key, name);
        return;
    }

    auto it = m_plots.find(key);
    if (it == m_plots.end())
        return;

    delete it->card;
    m_plots.erase(it);
    updateEmptyState();
}

void SignalPlotWindow::setTaggedHistory(quint64 key, const QString &name, const QVector<CanFrame> &frames)
{
    ensurePlot(key, name);
    auto it = m_plots.find(key);
    if (it == m_plots.end() || !it->plot)
        return;
    it->plot->setFrames(frames);
    if (it->title)
        it->title->setText(QStringLiteral("%1  [0x%2]  –  History: %3").arg(name).arg(key, 0, 16).arg(frames.size()));
}

void SignalPlotWindow::appendTaggedFrame(quint64 key, const QString &name, const CanFrame &frame)
{
    ensurePlot(key, name);
    auto it = m_plots.find(key);
    if (it != m_plots.end() && it->plot)
    {
        it->plot->append(frame);
        if (it->title)
            it->title->setText(QStringLiteral("%1  [0x%2]  –  History: %3").arg(name).arg(key, 0, 16).arg(it->plot->sampleCount()));
    }
}

void SignalPlotWindow::clearAll()
{
    for (auto it = m_plots.begin(); it != m_plots.end(); ++it)
        delete it->card;
    m_plots.clear();
    updateEmptyState();
}

void SignalPlotWindow::updateEmptyState()
{
    if (m_emptyLabel)
        m_emptyLabel->setVisible(m_plots.isEmpty());
}
