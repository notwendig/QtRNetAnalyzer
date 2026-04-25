#pragma once

#include "signalhistorymodel.h"

#include <QColor>
#include <QPoint>
#include <QWidget>

class SignalPlotWidget final : public QWidget
{
    Q_OBJECT

public:
    explicit SignalPlotWidget(QWidget *parent = nullptr);

    void setModel(SignalHistoryModel *model);
    void setPaused(bool paused);
    bool isPaused() const { return m_paused; }
    static QColor colorForSignalKey(quint64 key);

public slots:
    void resetZoom();
    void refreshView();

signals:
    void pauseChanged(bool paused);
    void selectionZoomed(double startSec, double endSec);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    QRect plotRect() const;
    double xToTime(int x) const;
    int timeToX(double timeSec) const;
    int valueToY(double value, double minValue, double maxValue) const;

    void updateLiveWindow();
    void calculateVisibleRange(double *minValue, double *maxValue) const;
    void drawBackground(QPainter &painter, const QRect &plot) const;
    void drawTimeAxis(QPainter &painter, const QRect &plot) const;
    void drawSignals(QPainter &painter, const QRect &plot) const;
    void drawSelection(QPainter &painter, const QRect &plot) const;
    void drawCrosshair(QPainter &painter, const QRect &plot) const;
    void drawLegend(QPainter &painter, const QRect &plot) const;
    QString cursorText(double timeSec) const;

private:
    SignalHistoryModel *m_model = nullptr;

    double m_viewStart = 0.0;
    double m_viewEnd = 10.0;
    double m_liveWindowSec = 30.0;

    bool m_paused = false;
    bool m_hasManualZoom = false;

    bool m_selecting = false;
    QPoint m_selectionStart;
    QPoint m_selectionEnd;

    bool m_mouseInside = false;
    QPoint m_mousePos;
};
