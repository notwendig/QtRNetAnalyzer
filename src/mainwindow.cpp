#include "mainwindow.h"

#include "liveframedelegate.h"
#include "rnetframedelegate.h"

#include <QAbstractItemView>
#include <QAction>
#include <QAbstractTableModel>
#include <QCheckBox>
#include <QColor>
#include <QComboBox>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QHash>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QSpinBox>
#include <QSortFilterProxyModel>
#include <QStatusBar>
#include <QTableView>
#include <QTabWidget>
#include <QTextStream>
#include <QTimer>
#include <QVBoxLayout>

namespace {

struct BitrateItem {
    const char *label;
    UCHAR t0;
    UCHAR t1;
};

constexpr BitrateItem kBitrates[] = {
    {"1000 kbit/s", 0x00, 0x14},
    {"500 kbit/s", 0x00, 0x1C},
    {"250 kbit/s", 0x01, 0x1C},
    {"125 kbit/s (R-Net)", 0x03, 0x1C},
    {"100 kbit/s", 0x04, 0x1C},
    {"50 kbit/s", 0x09, 0x1C},
    };

QString formatFrameTypeLocal(const CanFrame &frame)
{
    return QStringLiteral("%1/%2")
    .arg(frame.extended ? QStringLiteral("EXT") : QStringLiteral("STD"))
        .arg(frame.remote ? QStringLiteral("RTR") : QStringLiteral("DATA"));
}

} // namespace

MainWindow::MainWindow(const QString &inputFile, QWidget *parent)
    : QMainWindow(parent)
    , m_inputFile(inputFile)
    , m_simulationMode(!inputFile.trimmed().isEmpty())
    , m_worker(new ControlCanDeviceWorker(this))
{
    setWindowTitle(QStringLiteral("Qt6 ControlCAN Analyzer Pro"));
    resize(1400, 900);
    m_simulationTimer = new QTimer(this);
    m_simulationTimer->setInterval(10);
    connect(m_simulationTimer, &QTimer::timeout, this, &MainWindow::replaySimulationTick);
    createSimulationMenu();
    setCentralWidget(createCentral());

    m_liveModel = new LiveFrameModel(this);
    m_rnetModel = new RNetFrameModel(this);

    m_liveProxy = new QSortFilterProxyModel(this);
    m_liveProxy->setSourceModel(m_liveModel);
    m_liveProxy->setDynamicSortFilter(false);

    m_rnetProxy = new QSortFilterProxyModel(this);
    m_rnetProxy->setSourceModel(m_rnetModel);
    m_rnetProxy->setDynamicSortFilter(false);

    m_liveView->setModel(m_liveProxy);
    m_liveView->setItemDelegate(new LiveFrameDelegate(m_liveView));
    m_rnetView->setModel(m_rnetProxy);
    m_rnetView->setItemDelegate(new RNetFrameDelegate(m_rnetView));

    connect(m_rnetModel, &RNetFrameModel::tagStateChanged, this, [this](quint64 key, const QString &name, bool enabled) {
        if (!m_signalView)
            return;

        if (!enabled) {
            m_taggedSignalSources.remove(key);
            m_signalView->removeSource(key);
            updateSignalViewAvailability();
            return;
        }

        m_taggedSignalSources.insert(key);
        updateSignalViewAvailability();

        const auto *history = m_rnetModel->historyForKey(key);
        if (!history)
            return;

        for (const auto &entry : *history) {
            if (entry)
                m_signalView->addFrame(key, name, *entry);
        }
    });

    connect(m_rnetModel, &RNetFrameModel::taggedFrameReceived, this, [this](quint64 key, const QString &name, const CanFrame &frame) {
        if (m_signalView)
            m_signalView->addFrame(key, name, frame);
    });

    connect(m_openBtn, &QPushButton::clicked, this, &MainWindow::openDevice);
    connect(m_closeBtn, &QPushButton::clicked, this, &MainWindow::closeDevice);
    connect(m_sendBtn, &QPushButton::clicked, this, &MainWindow::sendFrame);
    connect(m_logBtn, &QPushButton::clicked, this, &MainWindow::toggleLogging);
    connect(m_clearBtn, &QPushButton::clicked, this, &MainWindow::clearTables);
    connect(m_rnetPresetBtn, &QPushButton::clicked, this, &MainWindow::applyRNetPreset);

    connect(m_worker, &ControlCanDeviceWorker::frameBatchReady, this, &MainWindow::onFrameBatch);
    connect(m_worker, &ControlCanDeviceWorker::frameTransmitted, this, &MainWindow::onFrameTx);
    connect(m_worker, &ControlCanDeviceWorker::countersUpdated, this, &MainWindow::onCounters);
    connect(m_worker, &ControlCanDeviceWorker::statusMessage, this, &MainWindow::onStatusMessage);
    connect(m_worker, &ControlCanDeviceWorker::deviceStateChanged, this, &MainWindow::onDeviceStateChanged);

    applyRNetPreset();
    onDeviceStateChanged(false);

    if (m_simulationMode) {
        m_openBtn->setEnabled(false);
        m_closeBtn->setEnabled(false);
        m_sendBtn->setEnabled(false);

        QString error;
        if (!loadSimulationFile(m_inputFile, &error)) {
            QMessageBox::warning(this, QStringLiteral("Simulation source"), error);
            onStatusMessage(error, true);
        } else {
            statusBar()->showMessage(QStringLiteral("Simulation source selected. Use Simulation > Start once/repeat."));
        }
    } else {
        statusBar()->showMessage(QStringLiteral("Ready"));
    }

    updateSimulationActions();
}

MainWindow::~MainWindow()
{
    stopSimulation();
    m_logger.stop();
    m_worker->closeDevice();
    delete m_signalView;
    m_signalView = nullptr;
}

void MainWindow::createSimulationMenu()
{
    auto *menu = menuBar()->addMenu(QStringLiteral("&Simulation"));

    m_simSelectAction = menu->addAction(QStringLiteral("Select source..."), this, &MainWindow::selectSimulationSource);
    menu->addSeparator();
    m_simStartRepeatAction = menu->addAction(QStringLiteral("Start repeat"), this, &MainWindow::startSimulationRepeat);
    m_simStartOnceAction = menu->addAction(QStringLiteral("Start once"), this, &MainWindow::startSimulationOnce);
    m_simStopAction = menu->addAction(QStringLiteral("Stop"), this, &MainWindow::stopSimulation);

    updateSimulationActions();
}

void MainWindow::updateSimulationActions()
{
    const bool hasSource = !m_simulationFrames.isEmpty();

    if (m_simSelectAction)
        m_simSelectAction->setEnabled(!m_simulationRunning);
    if (m_simStartRepeatAction)
        m_simStartRepeatAction->setEnabled(hasSource && !m_simulationRunning);
    if (m_simStartOnceAction)
        m_simStartOnceAction->setEnabled(hasSource && !m_simulationRunning);
    if (m_simStopAction)
        m_simStopAction->setEnabled(m_simulationRunning);
}

QWidget *MainWindow::createCentral()
{
    auto *root = new QWidget(this);
    auto *layout = new QVBoxLayout(root);

    auto *top = new QHBoxLayout;
    top->addWidget(createDeviceGroup(), 2);
    top->addWidget(createTransmitGroup(), 1);
    layout->addLayout(top);
    layout->addWidget(createViews(), 1);

    return root;
}

QGroupBox *MainWindow::createDeviceGroup()
{
    auto *box = new QGroupBox(QStringLiteral("Device / Capture"), this);
    auto *grid = new QGridLayout(box);

    m_deviceIndex = new QSpinBox(box);
    m_deviceIndex->setRange(0, 15);

    m_bitrate = new QComboBox(box);
    for (const auto &item : kBitrates)
        m_bitrate->addItem(QString::fromLatin1(item.label));

    m_batchSize = new QSpinBox(box);
    m_batchSize->setRange(64, 4000);
    m_batchSize->setValue(2000);

    m_pollDelay = new QSpinBox(box);
    m_pollDelay->setRange(1, 100);
    m_pollDelay->setValue(10);

    m_openBtn = new QPushButton(QStringLiteral("Open"), box);
    m_closeBtn = new QPushButton(QStringLiteral("Close"), box);
    m_logBtn = new QPushButton(QStringLiteral("Start CSV Log"), box);
    m_clearBtn = new QPushButton(QStringLiteral("Clear Views"), box);
    m_rnetPresetBtn = new QPushButton(QStringLiteral("Apply R-Net Preset"), box);
    m_summary = new QLabel(QStringLiteral("RX0=0 RX1=0 TX0=0 TX1=0 ERR0=0 ERR1=0"), box);

    grid->addWidget(new QLabel(QStringLiteral("Device index"), box), 0, 0);
    grid->addWidget(m_deviceIndex, 0, 1);
    grid->addWidget(new QLabel(QStringLiteral("Bitrate"), box), 0, 2);
    grid->addWidget(m_bitrate, 0, 3);
    grid->addWidget(new QLabel(QStringLiteral("Receive batch"), box), 1, 0);
    grid->addWidget(m_batchSize, 1, 1);
    grid->addWidget(new QLabel(QStringLiteral("Poll delay ms"), box), 1, 2);
    grid->addWidget(m_pollDelay, 1, 3);

    grid->addWidget(createChannelGroup(QStringLiteral("Channel 1"), m_ch0, 0), 2, 0, 1, 2);
    grid->addWidget(createChannelGroup(QStringLiteral("Channel 2"), m_ch1, 1), 2, 2, 1, 2);

    auto *buttons = new QHBoxLayout;
    buttons->addWidget(m_openBtn);
    buttons->addWidget(m_closeBtn);
    buttons->addWidget(m_logBtn);
    buttons->addWidget(m_clearBtn);
    buttons->addWidget(m_rnetPresetBtn);

    grid->addLayout(buttons, 3, 0, 1, 4);
    grid->addWidget(m_summary, 4, 0, 1, 4);

    return box;
}

QGroupBox *MainWindow::createChannelGroup(const QString &title, ChannelWidgets &widgets, int channel)
{
    auto *box = new QGroupBox(title, this);
    auto *form = new QFormLayout(box);

    widgets.enabled = new QCheckBox(QStringLiteral("Enable CAN%1").arg(channel + 1), box);
    widgets.enabled->setChecked(true);

    widgets.mode = new QComboBox(box);
    widgets.mode->addItem(QStringLiteral("Listen-only"), 1);
    widgets.mode->addItem(QStringLiteral("Normal"), 0);
    widgets.mode->addItem(QStringLiteral("Loopback"), 2);

    widgets.filter = new QComboBox(box);
    widgets.filter->addItem(QStringLiteral("All types"), 1);
    widgets.filter->addItem(QStringLiteral("Filter both STD+EXT"), 0);
    widgets.filter->addItem(QStringLiteral("STD only"), 2);
    widgets.filter->addItem(QStringLiteral("EXT only"), 3);

    widgets.accCode = new QLineEdit(QStringLiteral("0x00000000"), box);
    widgets.accMask = new QLineEdit(QStringLiteral("0xFFFFFFFF"), box);
    widgets.state = new QLabel(QStringLiteral("closed"), box);
    setStatusLamp(widgets.state, QStringLiteral("closed"), QStringLiteral("#666"));

    form->addRow(widgets.enabled);
    form->addRow(QStringLiteral("Mode"), widgets.mode);
    form->addRow(QStringLiteral("Filter"), widgets.filter);
    form->addRow(QStringLiteral("AccCode"), widgets.accCode);
    form->addRow(QStringLiteral("AccMask"), widgets.accMask);
    form->addRow(QStringLiteral("Status"), widgets.state);

    return box;
}

QGroupBox *MainWindow::createTransmitGroup()
{
    auto *box = new QGroupBox(QStringLiteral("Transmit"), this);
    auto *form = new QFormLayout(box);

    m_txChannel = new QComboBox(box);
    m_txChannel->addItem(QStringLiteral("CAN1"), 0);
    m_txChannel->addItem(QStringLiteral("CAN2"), 1);

    m_txId = new QLineEdit(QStringLiteral("0x000"), box);
    m_txExtended = new QCheckBox(QStringLiteral("Extended 29-bit"), box);
    m_txRemote = new QCheckBox(QStringLiteral("Remote frame"), box);
    m_txData = new QLineEdit(QStringLiteral("00 00 00 00 00 00 00 00"), box);
    m_sendBtn = new QPushButton(QStringLiteral("Send"), box);

    form->addRow(QStringLiteral("Channel"), m_txChannel);
    form->addRow(QStringLiteral("CAN ID"), m_txId);
    form->addRow(m_txExtended);
    form->addRow(m_txRemote);
    form->addRow(QStringLiteral("Data bytes"), m_txData);
    form->addRow(m_sendBtn);

    return box;
}

QTabWidget *MainWindow::createViews()
{
    auto *tabs = new QTabWidget(this);
    tabs->addTab(createLiveTab(), QStringLiteral("Live Frames"));
    tabs->addTab(createRNetTab(), QStringLiteral("R-Net View"));
    tabs->addTab(createLogTab(), QStringLiteral("Status / Log"));
    return tabs;
}

QWidget *MainWindow::createLiveTab()
{
    auto *w = new QWidget(this);
    auto *layout = new QVBoxLayout(w);

    m_liveView = new QTableView(w);
    m_liveView->setAlternatingRowColors(true);
    m_liveView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_liveView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_liveView->setSortingEnabled(true);
    m_liveView->verticalHeader()->setVisible(false);
    m_liveView->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_liveView->horizontalHeader()->setStretchLastSection(true);

    layout->addWidget(m_liveView);
    return w;
}

QWidget *MainWindow::createRNetTab()
{
    auto *w = new QWidget(this);
    auto *layout = new QVBoxLayout(w);

    auto *top = new QHBoxLayout;

    auto *hint = new QLabel(
        QStringLiteral("Optimized for 125 kbit/s R-Net capture and candump replay. Check R-Net rows in the Plot column, then open Signal View."),
        w);
    hint->setWordWrap(true);

    m_signalViewBtn = new QPushButton(QStringLiteral("Signal View"), w);
    m_signalViewBtn->setEnabled(false);
    m_signalViewBtn->setToolTip(QStringLiteral("Enabled when at least one R-Net row is checked for plotting."));

    top->addWidget(hint, 1);
    top->addWidget(m_signalViewBtn, 0, Qt::AlignTop);

    m_signalView = new SignalViewWindow(nullptr);
    m_signalView->setWindowTitle(QStringLiteral("QtRNetAnalyzer - Signal View"));
    m_signalView->resize(1100, 700);
    m_signalView->setAttribute(Qt::WA_DeleteOnClose, false);
    m_signalView->setInputEnabled(false);

    connect(m_signalViewBtn, &QPushButton::clicked, this, &MainWindow::openSignalViewWindow);

    m_rnetView = new QTableView(w);
    m_rnetView->setAlternatingRowColors(true);
    m_rnetView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_rnetView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_rnetView->setSortingEnabled(true);
    m_rnetView->verticalHeader()->setVisible(false);
    m_rnetView->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_rnetView->horizontalHeader()->setStretchLastSection(true);

    layout->addLayout(top);
    layout->addWidget(m_rnetView);
    return w;
}

void MainWindow::openSignalViewWindow()
{
    if (!m_signalView || m_taggedSignalSources.isEmpty())
        return;

    m_signalView->setInputEnabled(true);
    m_signalView->show();
    m_signalView->raise();
    m_signalView->activateWindow();
}

void MainWindow::updateSignalViewAvailability()
{
    const bool available = !m_taggedSignalSources.isEmpty();
    if (m_signalViewBtn)
        m_signalViewBtn->setEnabled(available);
    if (m_signalView)
        m_signalView->setInputEnabled(available);
}

QWidget *MainWindow::createLogTab()
{
    auto *w = new QWidget(this);
    auto *layout = new QVBoxLayout(w);

    m_logView = new QPlainTextEdit(w);
    m_logView->setReadOnly(true);

    layout->addWidget(m_logView);
    return w;
}

bool MainWindow::parseHexUInt(const QString &text, quint32 *value)
{
    bool ok = false;
    const QString cleaned = text.trimmed().startsWith(QStringLiteral("0x"), Qt::CaseInsensitive)
                                ? text.trimmed().mid(2)
                                : text.trimmed();
    const quint32 parsed = cleaned.toUInt(&ok, 16);
    if (ok && value)
        *value = parsed;
    return ok;
}

QByteArray MainWindow::parseHexBytes(const QString &text, bool *ok)
{
    QString cleaned = text;
    cleaned.replace(',', ' ');
    cleaned = cleaned.simplified().remove(' ');

    if (cleaned.isEmpty()) {
        if (ok)
            *ok = true;
        return {};
    }

    if ((cleaned.size() % 2) != 0) {
        if (ok)
            *ok = false;
        return {};
    }

    const QByteArray bytes = QByteArray::fromHex(cleaned.toLatin1());
    if (ok)
        *ok = (bytes.size() * 2 == cleaned.size()) && bytes.size() <= 8;
    return bytes;
}

QString MainWindow::frameTypeText(const CanFrame &frame)
{
    return formatFrameTypeLocal(frame);
}

QString MainWindow::frameIdText(const CanFrame &frame)
{
    return "idtext";
}

bool MainWindow::parseSimulationLine(const QString &line, quint32 syntheticTs, CanFrame *frame)
{
    if (!frame)
        return false;

    const QString trimmed = line.trimmed();
    if (trimmed.isEmpty() || trimmed.startsWith('#') || trimmed.startsWith(QStringLiteral("--")))
        return false;

    QString frameToken;

    // candump formats:
    //   (1622641895.104166) can0 02000300#0000
    //   can0 02000300#0000
    QString payload = trimmed;
    if (payload.startsWith('(')) {
        const int closePos = payload.indexOf(')');
        if (closePos > 0)
            payload = payload.mid(closePos + 1).trimmed();
    }

    const QStringList parts = payload.split(QRegularExpression(QStringLiteral("\\s+")),
                                            Qt::SkipEmptyParts);

    for (const QString &part : parts) {
        if (part.contains('#')) {
            frameToken = part;
            break;
        }
    }

    // Lua / script / text formats:
    // scan for the first CAN frame token embedded in quotes/comments,
    // for example "02000300#0000" or cansend can0 1C0C0000#32.
    if (frameToken.isEmpty()) {
        static const QRegularExpression tokenRe(
            QStringLiteral("([0-9A-Fa-f]{1,8})#([0-9A-Fa-f]{0,16}|R|r)"));
        const QRegularExpressionMatch match = tokenRe.match(trimmed);
        if (!match.hasMatch())
            return false;
        frameToken = match.captured(0);
    }

    frameToken = frameToken.trimmed();
    frameToken.remove('"');
    frameToken.remove('\'');
    frameToken.remove(',');
    frameToken.remove(';');

    const int hashPos = frameToken.indexOf('#');
    if (hashPos <= 0)
        return false;

    const QString idText = frameToken.left(hashPos).trimmed();
    const QString dataPart = frameToken.mid(hashPos + 1).trimmed();

    quint32 id = 0;
    if (!parseHexUInt(idText, &id))
        return false;

    CanFrame f;
    f.hostTime = QDateTime::currentDateTime().toString(Qt::ISODateWithMs);
    f.hwTimestamp = syntheticTs;
    f.id = id;
    f.extended = idText.size() > 3;
    f.remote = (dataPart.compare(QStringLiteral("R"), Qt::CaseInsensitive) == 0);
    f.error = false;

    if (f.remote) {
        f.data.clear();
    } else {
        bool okData = false;
        f.data = parseHexBytes(dataPart, &okData);
        if (!okData)
            return false;
    }

    *frame = f;
    return true;
}

bool MainWindow::parseCandumpLine(const QString &line, quint32 syntheticTs, CanFrame *frame)
{
    return parseSimulationLine(line, syntheticTs, frame);
}

bool MainWindow::loadSimulationFile(const QString &path, QString *error)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (error)
            *error = QStringLiteral("Cannot open simulation source: %1").arg(path);
        return false;
    }

    QTextStream in(&file);

    QVector<CanFrame> frames;
    frames.reserve(20000);

    quint32 syntheticTs = 0;

    while (true) {
        const QString line = in.readLine();
        if (line.isNull())
            break;

        CanFrame frame;
        if (!parseSimulationLine(line, syntheticTs, &frame))
            continue;

        frames.push_back(frame);
        syntheticTs += 100;
    }

    if (frames.isEmpty()) {
        if (error)
            *error = QStringLiteral("No valid CAN frames found in simulation source: %1").arg(path);
        return false;
    }

    m_inputFile = path;
    m_simulationMode = true;
    m_simulationFrames = std::move(frames);
    m_simulationIndex = 0;

    onStatusMessage(QStringLiteral("Simulation source selected: %1 (%2 frames)")
                        .arg(path)
                        .arg(m_simulationFrames.size()),
                    false);
    updateSimulationActions();
    return true;
}

DeviceOpenConfig MainWindow::currentConfigFromUi(bool *ok, QString *error) const
{
    DeviceOpenConfig cfg;
    cfg.deviceIndex = static_cast<DWORD>(m_deviceIndex->value());
    cfg.receiveBatch = m_batchSize->value();
    cfg.pollDelayMs = m_pollDelay->value();

    const int bitrateIndex = m_bitrate->currentIndex();
    const auto &rate = kBitrates[bitrateIndex >= 0 ? bitrateIndex : 0];

    auto fillChannel = [&](const ChannelWidgets &w, ChannelConfig &c, int index) -> bool {
        quint32 accCode = 0, accMask = 0;
        if (!parseHexUInt(w.accCode->text(), &accCode)) {
            if (error)
                *error = QStringLiteral("Invalid AccCode on CAN%1").arg(index + 1);
            return false;
        }
        if (!parseHexUInt(w.accMask->text(), &accMask)) {
            if (error)
                *error = QStringLiteral("Invalid AccMask on CAN%1").arg(index + 1);
            return false;
        }
        c.enabled = w.enabled->isChecked();
        c.canIndex = static_cast<DWORD>(index);
        c.accCode = accCode;
        c.accMask = accMask;
        c.filter = static_cast<UCHAR>(w.filter->currentData().toUInt());
        c.mode = static_cast<UCHAR>(w.mode->currentData().toUInt());
        c.timing0 = rate.t0;
        c.timing1 = rate.t1;
        return true;
    };

    bool localOk = fillChannel(m_ch0, cfg.channel0, 0) && fillChannel(m_ch1, cfg.channel1, 1);
    if (ok)
        *ok = localOk;
    return cfg;
}

void MainWindow::openDevice()
{
    if (m_simulationMode) {
        QMessageBox::information(this, QStringLiteral("Simulation mode"),
                                 QStringLiteral("Live CAN is disabled while replaying candump input."));
        return;
    }

    bool ok = false;
    QString error;
    const DeviceOpenConfig cfg = currentConfigFromUi(&ok, &error);
    if (!ok) {
        QMessageBox::warning(this, QStringLiteral("Config error"), error);
        return;
    }
    if (!m_worker->openDevice(cfg, &error)) {
        QMessageBox::warning(this, QStringLiteral("Open failed"), error);
    }
}

void MainWindow::closeDevice()
{
    m_worker->closeDevice();
}

void MainWindow::sendFrame()
{
    if (m_simulationMode) {
        QMessageBox::information(this, QStringLiteral("Simulation mode"),
                                 QStringLiteral("Transmit is disabled while replaying candump input."));
        return;
    }

    quint32 id = 0;
    if (!parseHexUInt(m_txId->text(), &id)) {
        QMessageBox::warning(this, QStringLiteral("Invalid ID"), QStringLiteral("CAN ID must be hex."));
        return;
    }

    bool ok = false;
    const QByteArray data = parseHexBytes(m_txData->text(), &ok);
    if (!ok) {
        QMessageBox::warning(this, QStringLiteral("Invalid data"),
                             QStringLiteral("Data must be 0-8 bytes in hex, e.g. 01 02 03."));
        return;
    }

    m_worker->queueTransmit(m_txChannel->currentData().toInt(),
                            id,
                            data,
                            m_txExtended->isChecked(),
                            m_txRemote->isChecked());
}

void MainWindow::toggleLogging()
{
    if (m_logger.isActive()) {
        m_logger.stop();
        m_logBtn->setText(QStringLiteral("Start CSV Log"));
        onStatusMessage(QStringLiteral("CSV logging stopped"), false);
        return;
    }

    const QString path = QFileDialog::getSaveFileName(
        this,
        QStringLiteral("Save CSV log"),
        QDir::homePath() + QStringLiteral("/controlcan_capture.csv"),
        QStringLiteral("CSV Files (*.csv)"));

    if (path.isEmpty())
        return;

    QString error;
    if (!m_logger.start(path, &error)) {
        QMessageBox::warning(this, QStringLiteral("Logger"), error);
        return;
    }

    m_logBtn->setText(QStringLiteral("Stop CSV Log"));
    onStatusMessage(QStringLiteral("CSV logging to %1").arg(path), false);
}

void MainWindow::clearTables()
{
    m_liveModel->clear();
    static_cast<RNetFrameModel *>(m_rnetModel)->clear();
    m_taggedSignalSources.clear();
    if (m_signalView)
        m_signalView->clear();
    updateSignalViewAvailability();
    m_logView->clear();
    m_displayedFrames = 0;
}

void MainWindow::applyRNetPreset()
{
    m_bitrate->setCurrentIndex(3);
    m_ch0.enabled->setChecked(true);
    m_ch1.enabled->setChecked(true);
    m_ch0.mode->setCurrentIndex(0);
    m_ch1.mode->setCurrentIndex(0);
    m_ch0.filter->setCurrentIndex(0);
    m_ch1.filter->setCurrentIndex(0);
    m_ch0.accCode->setText(QStringLiteral("0x00000000"));
    m_ch1.accCode->setText(QStringLiteral("0x00000000"));
    m_ch0.accMask->setText(QStringLiteral("0xFFFFFFFF"));
    m_ch1.accMask->setText(QStringLiteral("0xFFFFFFFF"));
    onStatusMessage(QStringLiteral("Applied 125 kbit/s R-Net preset with both channels in listen-only mode."), false);
}

void MainWindow::onFrameBatch(const QVector<CanFrame> &frames)
{
    if (frames.isEmpty())
        return;

    auto *rnetModel = static_cast<RNetFrameModel *>(m_rnetModel);

    m_liveModel->addFrames(frames);

    for (const CanFrame &frame : frames) {
        rnetModel->addFrame(frame);

        if (m_logger.isActive())
            m_logger.write(frame);

        ++m_displayedFrames;
    }

    if (m_liveModel->rowCount() > 0)
        m_liveView->scrollToBottom();
}

void MainWindow::onFrameTx(const CanFrame &frame)
{
    QVector<CanFrame> one;
    one.push_back(frame);
    onFrameBatch(one);
}

void MainWindow::setStatusLamp(QLabel *label, const QString &text, const QString &color)
{
    label->setText(text);
    label->setStyleSheet(
        QStringLiteral("QLabel { background:%1; color:white; padding:4px 8px; border-radius:8px; }")
            .arg(color));
}

void MainWindow::onCounters(quint64 rx0, quint64 rx1, quint64 tx0, quint64 tx1, quint64 err0, quint64 err1)
{
    m_summary->setText(QStringLiteral("RX0=%1  RX1=%2  TX0=%3  TX1=%4  ERR0=%5  ERR1=%6  Displayed=%7")
                           .arg(rx0).arg(rx1).arg(tx0).arg(tx1).arg(err0).arg(err1).arg(m_displayedFrames));

    setStatusLamp(m_ch0.state,
                  err0 == 0 ? QStringLiteral("active") : QStringLiteral("warn"),
                  err0 == 0 ? QStringLiteral("#2e7d32") : QStringLiteral("#ef6c00"));

    setStatusLamp(m_ch1.state,
                  err1 == 0 ? QStringLiteral("active") : QStringLiteral("warn"),
                  err1 == 0 ? QStringLiteral("#2e7d32") : QStringLiteral("#ef6c00"));
}

void MainWindow::onStatusMessage(const QString &message, bool error)
{
    const QString line = QStringLiteral("[%1] %2")
    .arg(QDateTime::currentDateTime().toString(Qt::ISODateWithMs), message);
    m_logView->appendPlainText(line);
    statusBar()->showMessage(message, 5000);

    if (error)
        m_logView->appendPlainText(QStringLiteral("  -> check device open state, channel config, or cable/bus state"));
}

void MainWindow::onDeviceStateChanged(bool open)
{
    if (!m_simulationMode) {
        m_openBtn->setEnabled(!open);
        m_closeBtn->setEnabled(open);
        m_sendBtn->setEnabled(open);
    }

    if (!open) {
        setStatusLamp(m_ch0.state, QStringLiteral("closed"), QStringLiteral("#666"));
        setStatusLamp(m_ch1.state, QStringLiteral("closed"), QStringLiteral("#666"));
    }
}

void MainWindow::selectSimulationSource()
{
    const QString path = QFileDialog::getOpenFileName(
        this,
        QStringLiteral("Select simulation source"),
        m_inputFile.isEmpty() ? QDir::homePath() : m_inputFile,
        QStringLiteral("Simulation sources (*.txt *.log *.candump *.lua);;Candump text (*.txt *.log *.candump);;Lua scripts (*.lua);;All files (*)"));

    if (path.isEmpty())
        return;

    QString error;
    if (!loadSimulationFile(path, &error)) {
        QMessageBox::warning(this, QStringLiteral("Simulation source"), error);
        onStatusMessage(error, true);
    }
}

void MainWindow::startSimulationOnce()
{
    if (m_simulationFrames.isEmpty())
        return;

    // A one-shot replay must never inherit a previous repeat run.
    // Stop any active/queued replay timer before changing mode.
    if (m_simulationTimer)
        m_simulationTimer->stop();

    m_simulationRepeat = false;
    m_simulationRunning = false;
    m_simulationIndex = 0;

    clearTables();

    m_simulationRepeat = false;
    m_simulationRunning = true;
    m_simulationIndex = 0;
    m_simulationTimer->start();

    onStatusMessage(QStringLiteral("Simulation started once: %1").arg(m_inputFile), false);
    updateSimulationActions();
}

void MainWindow::startSimulationRepeat()
{
    if (m_simulationFrames.isEmpty())
        return;

    if (m_simulationTimer)
        m_simulationTimer->stop();

    m_simulationRepeat = false;
    m_simulationRunning = false;
    m_simulationIndex = 0;

    clearTables();

    m_simulationRepeat = true;
    m_simulationRunning = true;
    m_simulationIndex = 0;
    m_simulationTimer->start();

    onStatusMessage(QStringLiteral("Simulation started repeat: %1").arg(m_inputFile), false);
    updateSimulationActions();
}

void MainWindow::stopSimulation()
{
    if (m_simulationTimer)
        m_simulationTimer->stop();

    if (m_simulationRunning)
        onStatusMessage(QStringLiteral("Simulation stopped"), false);

    m_simulationRunning = false;
    m_simulationRepeat = false;
    updateSimulationActions();
}

void MainWindow::replaySimulationTick()
{
    if (!m_simulationRunning || m_simulationFrames.isEmpty()) {
        stopSimulation();
        return;
    }

    constexpr qsizetype kFramesPerTick = 80;

    QVector<CanFrame> batch;
    batch.reserve(kFramesPerTick);

    for (qsizetype i = 0; i < kFramesPerTick; ++i) {
        if (m_simulationIndex >= m_simulationFrames.size()) {
            if (m_simulationRepeat) {
                m_simulationIndex = 0;
            } else {
                break;
            }
        }

        if (m_simulationIndex < m_simulationFrames.size())
            batch.push_back(m_simulationFrames.at(m_simulationIndex++));
    }

    if (!batch.isEmpty())
        onFrameBatch(batch);

    onCounters(static_cast<quint64>(m_simulationIndex), 0, 0, 0, 0, 0);

    if (!m_simulationRepeat && m_simulationIndex >= m_simulationFrames.size()) {
        stopSimulation();
        onStatusMessage(QStringLiteral("Simulation finished: %1 frames").arg(m_simulationFrames.size()), false);
    }
}

