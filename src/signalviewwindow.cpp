#include "signalviewwindow.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QList>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QVariant>
#include <QVBoxLayout>
#include <algorithm>

namespace {
constexpr int kSignalKeyRole = Qt::UserRole + 1;
}

SignalViewWindow::SignalViewWindow(QWidget *parent)
    : QWidget(parent)
{
    auto *root = new QHBoxLayout(this);

    auto *side = new QVBoxLayout;
    auto *title = new QLabel(QStringLiteral("Signals from checked R-Net rows"), this);
    title->setStyleSheet(QStringLiteral("font-weight: bold;"));

    m_signalList = new QListWidget(this);
    m_signalList->setMinimumWidth(260);

    m_resetZoom = new QPushButton(QStringLiteral("Reset Zoom"), this);
    m_status = new QLabel(QStringLiteral("Check R-Net rows to feed this view. Click plot to pause/live. Ctrl+drag to zoom."), this);
    m_status->setWordWrap(true);

    side->addWidget(title);
    side->addWidget(m_signalList, 1);
    side->addWidget(m_resetZoom);
    side->addWidget(m_status);

    m_plot = new SignalPlotWidget(this);
    m_plot->setModel(&m_history);

    root->addLayout(side);
    root->addWidget(m_plot, 1);

    connect(m_signalList, &QListWidget::itemChanged, this, &SignalViewWindow::onSignalItemChanged);
    connect(m_resetZoom, &QPushButton::clicked, m_plot, &SignalPlotWidget::resetZoom);
    connect(m_plot, &SignalPlotWidget::pauseChanged, this, &SignalViewWindow::onPauseChanged);
}

void SignalViewWindow::addFrame(quint64 sourceKey, const QString &sourceName, const CanFrame &frame)
{
    const int before = m_history.allSignals().size();
    m_history.addSamplesFromFrame(sourceKey, sourceName, frame);
    const int after = m_history.allSignals().size();

    if (after != before)
        rebuildSignalList();

    m_plot->refreshView();
}

void SignalViewWindow::removeSource(quint64 sourceKey)
{
    m_history.removeSource(sourceKey);
    rebuildSignalList();
    m_plot->refreshView();
}

void SignalViewWindow::clear()
{
    m_history.clear();
    m_signalList->clear();
    m_plot->resetZoom();
    m_plot->refreshView();
    m_status->setText(QStringLiteral("Signal history cleared."));
}

void SignalViewWindow::refreshSignals()
{
    rebuildSignalList();
    m_plot->refreshView();
}

void SignalViewWindow::onSignalItemChanged(QListWidgetItem *item)
{
    if (m_updatingList || !item)
        return;

    const quint64 key = itemKey(item);
    m_history.setSignalEnabled(key, item->checkState() == Qt::Checked);
    m_plot->refreshView();
}

void SignalViewWindow::onPauseChanged(bool paused)
{
    m_status->setText(paused
                          ? QStringLiteral("Paused. Ctrl+drag a marked area to zoom, right click resets zoom.")
                          : QStringLiteral("Live mode. The view follows newest data from checked R-Net rows."));
}

void SignalViewWindow::rebuildSignalList()
{
    m_updatingList = true;
    m_signalList->clear();

    QList<quint64> keys = m_history.allSignals().keys();
    std::sort(keys.begin(), keys.end());

    for (const quint64 key : keys) {
        const SignalHistory &history = m_history.allSignals().value(key);
        const QString unit = history.unit.isEmpty() ? QString() : QStringLiteral(" [%1]").arg(history.unit);
        auto *item = new QListWidgetItem(history.name + unit, m_signalList);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(history.enabled ? Qt::Checked : Qt::Unchecked);
        item->setData(kSignalKeyRole, QVariant::fromValue<qulonglong>(key));
    }

    m_updatingList = false;
}

quint64 SignalViewWindow::itemKey(const QListWidgetItem *item)
{
    return item ? item->data(kSignalKeyRole).toULongLong() : 0;
}
