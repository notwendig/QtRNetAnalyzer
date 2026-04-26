#include "signalviewwindow.h"

#include <QBrush>
#include <QHBoxLayout>
#include <QLabel>
#include <QMap>
#include <QPushButton>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>

#include <algorithm>

namespace
{
constexpr int kItemKindRole = Qt::UserRole + 1;
constexpr int kSignalKeyRole = Qt::UserRole + 2;
constexpr int kSourceKeyRole = Qt::UserRole + 3;

constexpr int kKindSource = 1;
constexpr int kKindSignal = 2;
}

SignalViewWindow::SignalViewWindow(QWidget *parent)
    : QWidget(parent)
{
    auto *root = new QHBoxLayout(this);

    auto *side = new QVBoxLayout;
    auto *title = new QLabel(QStringLiteral("Checked R-Net frames"), this);
    title->setStyleSheet(QStringLiteral("font-weight: bold;"));

    m_signalTree = new QTreeWidget(this);
    m_signalTree->setMinimumWidth(340);
    m_signalTree->setColumnCount(3);
    m_signalTree->setHeaderLabels({
        QStringLiteral("Signal"),
        QStringLiteral("Color"),
        QStringLiteral("Unit")
    });
    m_signalTree->setRootIsDecorated(true);
    m_signalTree->setAlternatingRowColors(true);

    m_resetZoom = new QPushButton(QStringLiteral("Reset Zoom"), this);

    m_status = new QLabel(
        QStringLiteral("Check at least one R-Net row in the R-Net table to enable Signal View selection."),
        this);
    m_status->setWordWrap(true);

    side->addWidget(title);
    side->addWidget(m_signalTree, 1);
    side->addWidget(m_resetZoom);
    side->addWidget(m_status);

    m_plot = new SignalPlotWidget(this);
    m_plot->setModel(&m_history);

    root->addLayout(side);
    root->addWidget(m_plot, 1);

    setInputEnabled(false);

    connect(m_signalTree, &QTreeWidget::itemChanged,
            this, &SignalViewWindow::onSignalItemChanged);
    connect(m_resetZoom, &QPushButton::clicked,
            m_plot, &SignalPlotWidget::resetZoom);
    connect(m_plot, &SignalPlotWidget::pauseChanged,
            this, &SignalViewWindow::onPauseChanged);
}

void SignalViewWindow::setInputEnabled(bool enabled)
{
    m_inputEnabled = enabled;

    if (m_signalTree)
        m_signalTree->setEnabled(enabled);
    if (m_resetZoom)
        m_resetZoom->setEnabled(enabled);
    if (m_plot)
        m_plot->setEnabled(enabled);

    if (!m_status)
        return;

    if (enabled) {
        m_status->setText(QStringLiteral(
            "Live mode.\nOnly checked signal channels from checked R-Net rows are plotted."));
    } else {
        m_status->setText(QStringLiteral(
            "Check at least one R-Net row in the R-Net table to enable Signal View selection."));
    }
}

void SignalViewWindow::addFrame(quint64 sourceKey, const QString &sourceName, const CanFrame &frame)
{
    const int before = m_history.allSignals().size();
    m_history.addSamplesFromFrame(sourceKey, sourceName, frame);
    const int after = m_history.allSignals().size();

    if (after != before)
        rebuildSignalTree();

    if (m_plot)
        m_plot->refreshView();
}

void SignalViewWindow::removeSource(quint64 sourceKey)
{
    m_history.removeSource(sourceKey);
    rebuildSignalTree();

    if (m_plot)
        m_plot->refreshView();
}

void SignalViewWindow::clear()
{
    m_history.clear();

    if (m_signalTree)
        m_signalTree->clear();

    if (m_plot) {
        m_plot->resetZoom();
        m_plot->refreshView();
    }

    setInputEnabled(false);
}

void SignalViewWindow::refreshSignals()
{
    rebuildSignalTree();

    if (m_plot)
        m_plot->refreshView();
}

void SignalViewWindow::onSignalItemChanged(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column)

    if (m_updatingTree || !item)
        return;

    m_updatingTree = true;

    if (isSourceItem(item)) {
        const bool enabled = item->checkState(0) == Qt::Checked;

        for (int i = 0; i < item->childCount(); ++i) {
            QTreeWidgetItem *child = item->child(i);
            child->setCheckState(0, enabled ? Qt::Checked : Qt::Unchecked);
            m_history.setSignalEnabled(itemKey(child), enabled);
        }
    } else if (isSignalItem(item)) {
        m_history.setSignalEnabled(itemKey(item), item->checkState(0) == Qt::Checked);

        QTreeWidgetItem *parent = item->parent();
        if (parent) {
            int checked = 0;
            for (int i = 0; i < parent->childCount(); ++i) {
                if (parent->child(i)->checkState(0) == Qt::Checked)
                    ++checked;
            }

            parent->setCheckState(
                0,
                checked == 0
                    ? Qt::Unchecked
                    : (checked == parent->childCount() ? Qt::Checked : Qt::PartiallyChecked));
        }
    }

    m_updatingTree = false;

    if (m_plot)
        m_plot->refreshView();
}

void SignalViewWindow::onPauseChanged(bool paused)
{
    if (!m_status)
        return;

    m_status->setText(paused
        ? QStringLiteral("Paused.\nDrag inside the plot to zoom, right click resets zoom.")
        : QStringLiteral("Live mode.\nOnly checked signal channels from checked R-Net rows are plotted."));
}

void SignalViewWindow::rebuildSignalTree()
{
    if (!m_signalTree)
        return;

    m_updatingTree = true;
    m_signalTree->clear();

    QMap<quint64, QList<quint64>> groups;
    QHash<quint64, QString> sourceNames;

    for (auto it = m_history.allSignals().constBegin(); it != m_history.allSignals().constEnd(); ++it) {
        groups[it.value().sourceKey].append(it.key());
        sourceNames.insert(
            it.value().sourceKey,
            it.value().sourceName.isEmpty() ? QStringLiteral("R-Net") : it.value().sourceName);
    }

    for (auto groupIt = groups.constBegin(); groupIt != groups.constEnd(); ++groupIt) {
        const quint64 sourceKey = groupIt.key();

        auto *sourceItem = new QTreeWidgetItem(m_signalTree);
        sourceItem->setText(0, sourceNames.value(sourceKey));
        sourceItem->setData(0, kItemKindRole, kKindSource);
        sourceItem->setData(0, kSourceKeyRole, QVariant::fromValue(sourceKey));
        sourceItem->setFlags(sourceItem->flags()
                             | Qt::ItemIsUserCheckable
                             | Qt::ItemIsAutoTristate);
        sourceItem->setExpanded(true);

        int enabledChildren = 0;
        QList<quint64> signalKeys = groupIt.value();
        std::sort(signalKeys.begin(), signalKeys.end());

        for (const quint64 signalKey : signalKeys) {
            const SignalHistory history = m_history.allSignals().value(signalKey);

            auto *signalItem = new QTreeWidgetItem(sourceItem);
            signalItem->setText(0, history.name);
            signalItem->setText(1, QStringLiteral("■"));
            signalItem->setForeground(1, QBrush(SignalPlotWidget::colorForSignalKey(signalKey)));
            signalItem->setText(2, history.unit);
            signalItem->setFlags(signalItem->flags() | Qt::ItemIsUserCheckable);
            signalItem->setCheckState(0, history.enabled ? Qt::Checked : Qt::Unchecked);
            signalItem->setData(0, kItemKindRole, kKindSignal);
            signalItem->setData(0, kSignalKeyRole, QVariant::fromValue(signalKey));

            if (history.enabled)
                ++enabledChildren;
        }

        sourceItem->setCheckState(
            0,
            enabledChildren == 0
                ? Qt::Unchecked
                : (enabledChildren == signalKeys.size() ? Qt::Checked : Qt::PartiallyChecked));
    }

    m_signalTree->resizeColumnToContents(0);
    m_signalTree->resizeColumnToContents(1);

    m_updatingTree = false;
}

quint64 SignalViewWindow::itemKey(const QTreeWidgetItem *item)
{
    return item ? item->data(0, kSignalKeyRole).toULongLong() : 0;
}

bool SignalViewWindow::isSignalItem(const QTreeWidgetItem *item)
{
    return item && item->data(0, kItemKindRole).toInt() == kKindSignal;
}

bool SignalViewWindow::isSourceItem(const QTreeWidgetItem *item)
{
    return item && item->data(0, kItemKindRole).toInt() == kKindSource;
}
