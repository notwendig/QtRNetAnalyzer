#pragma once

#include "signalhistorymodel.h"
#include "signalplotwidget.h"

#include <QString>
#include <QWidget>

class QLabel;
class QPushButton;
class QTreeWidget;
class QTreeWidgetItem;

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
    void setInputEnabled(bool enabled);

public slots:
    void refreshSignals();

private slots:
    void onSignalItemChanged(QTreeWidgetItem *item, int column);
    void onPauseChanged(bool paused);
    void onCursorTimeChanged(double timeSec, bool active);

private:
    void rebuildSignalTree();
    void updateTreeValuesAt(double timeSec, bool active);
    QString valueTextAt(quint64 signalKey, double timeSec) const;
    static quint64 itemKey(const QTreeWidgetItem *item);
    static bool isSignalItem(const QTreeWidgetItem *item);
    static bool isSourceItem(const QTreeWidgetItem *item);

private:
    SignalHistoryModel m_history;
    SignalPlotWidget *m_plot = nullptr;
    QTreeWidget *m_signalTree = nullptr;
    QLabel *m_status = nullptr;
    QPushButton *m_resetZoom = nullptr;

    bool m_updatingTree = false;
    bool m_inputEnabled = false;
};
