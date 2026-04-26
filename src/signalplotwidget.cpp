#include "signalplotwidget.h"

#include <QEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QToolTip>

#include <QtMath>
#include <limits>

QColor SignalPlotWidget::colorForSignalKey(quint64 key)
{
    const int hue = int((key * 97u) % 360u);
    return QColor::fromHsv(hue, 190, 235);
}

SignalPlotWidget::SignalPlotWidget(QWidget *parent)
    : QWidget(parent)
{
    setMouseTracking(true);
    setMinimumHeight(260);
    setFocusPolicy(Qt::StrongFocus);
}

void SignalPlotWidget::setModel(SignalHistoryModel *model)
{
    m_model = model;
    resetZoom();
}

void SignalPlotWidget::setPaused(bool paused)
{
    if (m_paused == paused)
        return;

    m_paused = paused;
    emit pauseChanged(m_paused);
    update();
}

void SignalPlotWidget::resetZoom()
{
    m_hasManualZoom = false;
    updateLiveWindow();
    update();
}

void SignalPlotWidget::refreshView()
{
    if (!m_paused && !m_hasManualZoom)
        updateLiveWindow();

    update();
}

void SignalPlotWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRect plot = plotRect();
    drawBackground(painter, plot);

    if (!m_model || !m_model->hasSamples()) {
        painter.setPen(QColor(180, 180, 180));
        painter.drawText(rect(), Qt::AlignCenter, QStringLiteral("No signal data yet"));
        return;
    }

    drawTimeAxis(painter, plot);
    drawSignals(painter, plot);
    drawSelection(painter, plot);
    drawCrosshair(painter, plot);
    drawLegend(painter, plot);
}

void SignalPlotWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_selecting = true;
        m_selectionStart = event->pos();
        m_selectionEnd = event->pos();
        update();
        return;
    }

    if (event->button() == Qt::RightButton)
        resetZoom();
}

void SignalPlotWidget::mouseMoveEvent(QMouseEvent *event)
{
    m_mouseInside = true;
    m_mousePos = event->pos();

    if (m_selecting)
        m_selectionEnd = event->pos();

    const QRect plot = plotRect();
    if (plot.contains(m_mousePos) && m_model && m_model->hasSamples()) {
        const double timeSec = xToTime(m_mousePos.x());
        QToolTip::showText(event->globalPosition().toPoint(), cursorText(timeSec), this);
        emit cursorTimeChanged(timeSec, true);
    } else {
        QToolTip::hideText();
        emit cursorTimeChanged(0.0, false);
    }

    update();
}

void SignalPlotWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (!m_selecting)
        return;

    const QRect plot = plotRect();
    const int x1 = qBound(plot.left(), m_selectionStart.x(), plot.right());
    const int x2 = qBound(plot.left(), m_selectionEnd.x(), plot.right());

    if (qAbs(x2 - x1) > 8) {
        const double t1 = xToTime(x1);
        const double t2 = xToTime(x2);
        m_viewStart = qMin(t1, t2);
        m_viewEnd = qMax(t1, t2);
        m_hasManualZoom = true;
        setPaused(true);
        emit selectionZoomed(m_viewStart, m_viewEnd);
    } else if (plot.contains(event->pos())) {
        setPaused(!m_paused);
    }

    m_selecting = false;
    update();
}

void SignalPlotWidget::leaveEvent(QEvent *event)
{
    QWidget::leaveEvent(event);
    m_mouseInside = false;
    QToolTip::hideText();
    emit cursorTimeChanged(0.0, false);
    update();
}

QRect SignalPlotWidget::plotRect() const
{
    return rect().adjusted(56, 12, -16, -34);
}

double SignalPlotWidget::xToTime(int x) const
{
    const QRect plot = plotRect();
    const double ratio = double(x - plot.left()) / qMax(1, plot.width());
    return m_viewStart + ratio * (m_viewEnd - m_viewStart);
}

int SignalPlotWidget::timeToX(double timeSec) const
{
    const QRect plot = plotRect();
    if (qFuzzyCompare(m_viewStart, m_viewEnd))
        return plot.left();

    const double ratio = (timeSec - m_viewStart) / (m_viewEnd - m_viewStart);
    return plot.left() + int(qRound(ratio * plot.width()));
}

int SignalPlotWidget::valueToY(double value, double minValue, double maxValue) const
{
    const QRect plot = plotRect();
    if (qFuzzyCompare(minValue, maxValue))
        return plot.center().y();

    const double ratio = (value - minValue) / (maxValue - minValue);
    return plot.bottom() - int(qRound(ratio * plot.height()));
}

void SignalPlotWidget::updateLiveWindow()
{
    if (!m_model || !m_model->hasSamples()) {
        m_viewStart = 0.0;
        m_viewEnd = 10.0;
        return;
    }

    m_viewEnd = m_model->maxTime();
    m_viewStart = qMax(m_model->minTime(), m_viewEnd - m_liveWindowSec);

    if (qFuzzyCompare(m_viewStart, m_viewEnd))
        m_viewEnd = m_viewStart + 1.0;
}

void SignalPlotWidget::calculateVisibleRange(double *minValue, double *maxValue) const
{
    double lo = 0.0;
    double hi = 1.0;
    bool have = false;

    if (m_model) {
        for (auto it = m_model->allSignals().constBegin(); it != m_model->allSignals().constEnd(); ++it) {
            const SignalHistory &history = it.value();
            if (!history.enabled)
                continue;

            for (const SignalSample &sample : history.samples) {
                if (sample.timeSec < m_viewStart || sample.timeSec > m_viewEnd)
                    continue;

                if (!have) {
                    lo = hi = sample.value;
                    have = true;
                } else {
                    lo = qMin(lo, sample.value);
                    hi = qMax(hi, sample.value);
                }
            }
        }
    }

    if (!have || qFuzzyCompare(lo, hi)) {
        lo -= 1.0;
        hi += 1.0;
    }

    const double padding = qMax(1.0, (hi - lo) * 0.08);
    *minValue = lo - padding;
    *maxValue = hi + padding;
}

void SignalPlotWidget::drawBackground(QPainter &painter, const QRect &plot) const
{
    painter.fillRect(rect(), QColor(18, 18, 20));
    painter.fillRect(plot, QColor(8, 10, 14));
    painter.setPen(QColor(45, 48, 55));
    painter.drawRect(plot.adjusted(0, 0, -1, -1));

    for (int i = 1; i < 5; ++i) {
        const int y = plot.top() + i * plot.height() / 5;
        painter.drawLine(plot.left(), y, plot.right(), y);
    }
}

void SignalPlotWidget::drawTimeAxis(QPainter &painter, const QRect &plot) const
{
    painter.setPen(QColor(135, 140, 150));

    const double span = qMax(0.001, m_viewEnd - m_viewStart);
    const double rawStep = span / 8.0;
    const double magnitude = qPow(10.0, qFloor(qLn(rawStep) / qLn(10.0)));

    double step = magnitude;
    if (rawStep / magnitude > 5.0)
        step = 10.0 * magnitude;
    else if (rawStep / magnitude > 2.0)
        step = 5.0 * magnitude;
    else if (rawStep / magnitude > 1.0)
        step = 2.0 * magnitude;

    const double firstTick = qCeil(m_viewStart / step) * step;
    for (double t = firstTick; t <= m_viewEnd; t += step) {
        const int x = timeToX(t);
        painter.setPen(QColor(42, 45, 52));
        painter.drawLine(x, plot.top(), x, plot.bottom());
        painter.setPen(QColor(150, 155, 165));
        painter.drawLine(x, plot.bottom(), x, plot.bottom() + 5);
        painter.drawText(x + 3, plot.bottom() + 20, QStringLiteral("%1 s").arg(t, 0, 'f', span < 2.0 ? 3 : 2));
    }
}

void SignalPlotWidget::drawSignals(QPainter &painter, const QRect &plot) const
{
    Q_UNUSED(plot)

    double minValue = 0.0;
    double maxValue = 1.0;
    calculateVisibleRange(&minValue, &maxValue);

    for (auto it = m_model->allSignals().constBegin(); it != m_model->allSignals().constEnd(); ++it) {
        const SignalHistory &history = it.value();
        if (!history.enabled || history.samples.isEmpty())
            continue;

        const QColor color = colorForSignalKey(it.key());
        painter.setPen(QPen(color, 1.7));

        QVector<int> visibleIndexes;
        visibleIndexes.reserve(qMin(history.samples.size(), width() * 3));
        for (int i = 0; i < history.samples.size(); ++i) {
            const SignalSample &sample = history.samples.at(i);
            if (sample.timeSec >= m_viewStart && sample.timeSec <= m_viewEnd)
                visibleIndexes.push_back(i);
        }

        if (visibleIndexes.isEmpty())
            continue;

        const int maxPoints = qMax(2000, width() * 3);
        const int stride = qMax(1, visibleIndexes.size() / maxPoints);

        QPainterPath path;
        bool started = false;
        QPointF lastPoint;

        for (int n = 0; n < visibleIndexes.size(); n += stride) {
            const SignalSample &sample = history.samples.at(visibleIndexes.at(n));
            const QPointF point(timeToX(sample.timeSec), valueToY(sample.value, minValue, maxValue));

            if (!started) {
                path.moveTo(point);
                started = true;
            } else {
                path.lineTo(point);
            }

            lastPoint = point;
        }

        // Always include the last visible sample so repeated zooming does not
        // silently drop the end of the visible curve because of stride rounding.
        if (!visibleIndexes.isEmpty()) {
            const SignalSample &sample = history.samples.at(visibleIndexes.last());
            const QPointF point(timeToX(sample.timeSec), valueToY(sample.value, minValue, maxValue));
            if (!started) {
                path.moveTo(point);
                started = true;
            } else if (point != lastPoint) {
                path.lineTo(point);
            }
            lastPoint = point;
        }

        if (started) {
            painter.drawPath(path);
            if (visibleIndexes.size() == 1)
                painter.drawEllipse(lastPoint, 2.4, 2.4);
        }
    }

    painter.setPen(QColor(150, 155, 165));
    painter.drawText(8, 22, QStringLiteral("%1").arg(maxValue, 0, 'g', 5));
    painter.drawText(8, height() - 38, QStringLiteral("%1").arg(minValue, 0, 'g', 5));
}

void SignalPlotWidget::drawSelection(QPainter &painter, const QRect &plot) const
{
    if (!m_selecting)
        return;

    QRect selection(m_selectionStart, m_selectionEnd);
    selection = selection.normalized().intersected(plot);
    painter.fillRect(selection, QColor(255, 255, 255, 45));
    painter.setPen(QPen(QColor(230, 230, 230), 1, Qt::DashLine));
    painter.drawRect(selection.adjusted(0, 0, -1, -1));
}

void SignalPlotWidget::drawCrosshair(QPainter &painter, const QRect &plot) const
{
    if (!m_mouseInside || !plot.contains(m_mousePos))
        return;

    painter.setPen(QPen(QColor(255, 220, 80), 1, Qt::DashLine));
    painter.drawLine(m_mousePos.x(), plot.top(), m_mousePos.x(), plot.bottom());

    painter.setPen(QColor(255, 235, 120));
    painter.drawText(m_mousePos.x() + 8, plot.top() + 18, QStringLiteral("t=%1 s").arg(xToTime(m_mousePos.x()), 0, 'f', 3));
}

void SignalPlotWidget::drawLegend(QPainter &painter, const QRect &plot) const
{
    Q_UNUSED(plot)

    painter.setPen(m_paused ? QColor(255, 190, 80) : QColor(120, 220, 120));
    painter.drawText(width() - 150, 22, m_paused ? QStringLiteral("PAUSED") : QStringLiteral("LIVE"));

    painter.setPen(QColor(150, 155, 165));
    painter.drawText(width() - 260, height() - 8, QStringLiteral("Drag: zoom | Click: pause/live | Right click: reset"));
}

QString SignalPlotWidget::cursorText(double timeSec) const
{
    return QStringLiteral("t = %1 s").arg(timeSec, 0, 'f', 3);
}
