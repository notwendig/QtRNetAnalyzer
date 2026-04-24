#pragma once

#include "signalhistorymodel.h"
#include "signalplotwidget.h"

#include <QWidget>

class QLabel;
class QListWidget;
class QListWidgetItem;
class QPushButton;

class SignalViewWindow final : public QWidget
{
    Q_OBJECT

public:
    explicit SignalViewWindow(QWidget *parent = nullptr);

    SignalHistoryModel *historyModel() { return &m_history; }
    const SignalHistoryModel *historyModel() const { return &m_history; }

    void addFrame(quint64 sourceKey, const QString &sourceName, const CanFrame &frame);
    void removeSource(quint64 sourceKey);
    void clear();

public slots:
    void refreshSignals();

private slots:
    void onSignalItemChanged(QListWidgetItem *item);
    void onPauseChanged(bool paused);

private:
    void rebuildSignalList();
    static quint64 itemKey(const QListWidgetItem *item);

private:
    SignalHistoryModel m_history;
    SignalPlotWidget *m_plot = nullptr;
    QListWidget *m_signalList = nullptr;
    QLabel *m_status = nullptr;
    QPushButton *m_resetZoom = nullptr;

    bool m_updatingList = false;
};
