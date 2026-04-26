#pragma once

#include <QMainWindow>
#include <QSet>

#include "canlogger.h"
#include "controlcandeviceworker.h"
#include "liveframemodel.h"
#include "rnetframemodel.h"
#include "signalviewwindow.h"

QT_BEGIN_NAMESPACE
class QCheckBox;
class QComboBox;
class QGroupBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QPlainTextEdit;
class QSpinBox;
class QTableView;
class QTabWidget;
QT_END_NAMESPACE

class QAbstractTableModel;
class QSortFilterProxyModel;

class MainWindow final : public QMainWindow
{
    Q_OBJECT
  public:
    explicit MainWindow(const QString &inputFile = QString(), QWidget *parent = nullptr);
    ~MainWindow() override;

  private slots:
    void openDevice();
    void closeDevice();
    void sendFrame();
    void toggleLogging();
    void clearTables();
    void applyRNetPreset();
    void onFrameBatch(const QVector<CanFrame> &frames);
    void onFrameTx(const CanFrame &frame);
    void onCounters(quint64 rx0, quint64 rx1, quint64 tx0, quint64 tx1, quint64 err0, quint64 err1);
    void onStatusMessage(const QString &message, bool error);
    void onDeviceStateChanged(bool open);
    void startSimulationIfRequested();

  private:
    struct ChannelWidgets {
        QCheckBox *enabled = nullptr;
        QComboBox *mode = nullptr;
        QComboBox *filter = nullptr;
        QLineEdit *accCode = nullptr;
        QLineEdit *accMask = nullptr;
        QLabel *state = nullptr;
    };

    QWidget *createCentral();
    QGroupBox *createDeviceGroup();
    QGroupBox *createChannelGroup(const QString &title, ChannelWidgets &widgets, int channel);
    QGroupBox *createTransmitGroup();
    QTabWidget *createViews();
    QWidget *createLiveTab();
    QWidget *createRNetTab();
    QWidget *createLogTab();
    void openSignalViewWindow();
    void updateSignalViewAvailability();

    DeviceOpenConfig currentConfigFromUi(bool *ok = nullptr, QString *error = nullptr) const;
    static bool parseHexUInt(const QString &text, quint32 *value);
    static QByteArray parseHexBytes(const QString &text, bool *ok);
    static QString frameTypeText(const CanFrame &frame);
    static QString frameIdText(const CanFrame &frame);

    static bool parseCandumpLine(const QString &line, quint32 syntheticTs, CanFrame *frame);
    bool replayCandumpFile(const QString &path, QString *error);

    void setStatusLamp(QLabel *label, const QString &text, const QString &color);

    QString m_inputFile;
    bool m_simulationMode = false;

    ControlCanDeviceWorker *m_worker = nullptr;
    CanLogger m_logger;

    QSpinBox *m_deviceIndex = nullptr;
    QComboBox *m_bitrate = nullptr;
    QSpinBox *m_batchSize = nullptr;
    QSpinBox *m_pollDelay = nullptr;
    QPushButton *m_openBtn = nullptr;
    QPushButton *m_closeBtn = nullptr;
    QPushButton *m_logBtn = nullptr;
    QPushButton *m_clearBtn = nullptr;
    QPushButton *m_rnetPresetBtn = nullptr;

    ChannelWidgets m_ch0;
    ChannelWidgets m_ch1;

    QComboBox *m_txChannel = nullptr;
    QLineEdit *m_txId = nullptr;
    QCheckBox *m_txExtended = nullptr;
    QCheckBox *m_txRemote = nullptr;
    QLineEdit *m_txData = nullptr;
    QPushButton *m_sendBtn = nullptr;

    QLabel *m_summary = nullptr;
    QTableView *m_liveView = nullptr;
    QTableView *m_rnetView = nullptr;
    QPlainTextEdit *m_logView = nullptr;
    SignalViewWindow *m_signalView = nullptr;
    QPushButton *m_signalViewBtn = nullptr;
    QSet<quint64> m_taggedSignalSources;

    LiveFrameModel *m_liveModel = nullptr;
    RNetFrameModel *m_rnetModel = nullptr;
    QSortFilterProxyModel *m_liveProxy = nullptr;
    QSortFilterProxyModel *m_rnetProxy = nullptr;

    quint64 m_displayedFrames = 0;
};
