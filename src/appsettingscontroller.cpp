#include "appsettingscontroller.h"

#include <QAbstractSpinBox>
#include <QAction>
#include <QApplication>
#include <QByteArray>
#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QDateTime>
#include <QDialog>
#include <QDialogButtonBox>
#include <QHash>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonValue>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QSplitter>
#include <QStatusBar>
#include <QTabWidget>
#include <QTextCursor>
#include <QtGlobal>
#include <QVBoxLayout>
#include <QStringList>
#include <QVariant>
#include <QWidget>

#include <algorithm>

namespace {
constexpr auto kSettingsKey = "ui/json";
constexpr auto kSchemaVersion = 1;

QString encodeState(const QByteArray &state)
{
  return QString::fromLatin1(state.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals));
}

QByteArray decodeState(const QString &text)
{
  return QByteArray::fromBase64(text.toLatin1(), QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
}

QString safeName(QString text)
{
  text.replace('/', '_');
  text.replace(' ', '_');
  return text;
}

QJsonObject stateFromRoot(const QJsonObject &root)
{
  const QJsonValue stateValue = root.value(QStringLiteral("state"));
  if (stateValue.isObject())
    return stateValue.toObject();
  return root;
}
} // namespace

AppSettingsController::AppSettingsController(QMainWindow *mainWindow)
  : QObject(mainWindow)
  , m_mainWindow(mainWindow)
{
  m_defaultState = captureState();
  installMenus();
  restore();

  connect(qApp, &QCoreApplication::aboutToQuit, this, [this]() {
    save();
    m_savedDuringShutdown = true;
  });
}

AppSettingsController::~AppSettingsController()
{
  if (!m_savedDuringShutdown)
    save();
}

void AppSettingsController::installMenus()
{
  if (!m_mainWindow)
    return;

  auto *settingsMenu = m_mainWindow->menuBar()->addMenu(tr("&Settings"));
  settingsMenu->addAction(tr("Edit JSON Settings..."), this, [this]() { editJsonSettings(); });
  settingsMenu->addAction(tr("Reset Settings"), this, [this]() { resetToDefaults(); });

  auto *helpMenu = m_mainWindow->menuBar()->addMenu(tr("&Help"));
  helpMenu->addAction(tr("&About QtRNetAnalyzer..."), this, [this]() { showAboutDialog(); });
}

void AppSettingsController::restore()
{
  const QJsonObject root = storedRoot();
  if (!root.isEmpty())
    applyRoot(root);
}

void AppSettingsController::save()
{
  storeRoot(captureRoot());
}

void AppSettingsController::resetToDefaults()
{
  if (!m_mainWindow)
    return;

  const auto answer = QMessageBox::question(
      m_mainWindow,
      tr("Reset settings"),
      tr("Reset QtRNetAnalyzer settings to the built-in defaults?\n\nThis clears the JSON settings stored via QSettings."),
      QMessageBox::Yes | QMessageBox::No,
      QMessageBox::No);

  if (answer != QMessageBox::Yes)
    return;

  m_settings.remove(QString::fromLatin1(kSettingsKey));
  m_settings.sync();
  applyState(m_defaultState);

  if (auto *bar = m_mainWindow->statusBar())
    bar->showMessage(tr("Settings reset to defaults"), 5000);
}

void AppSettingsController::editJsonSettings()
{
  if (!m_mainWindow)
    return;

  QDialog dialog(m_mainWindow);
  dialog.setWindowTitle(tr("Edit JSON Settings"));
  dialog.resize(900, 700);

  auto *layout = new QVBoxLayout(&dialog);
  auto *hint = new QLabel(
      tr("Edit the JSON below. It is stored by QSettings under key '%1'.\nInvalid JSON is rejected; valid JSON is applied immediately after Save.")
          .arg(QString::fromLatin1(kSettingsKey)),
      &dialog);
  hint->setWordWrap(true);
  layout->addWidget(hint);

  auto *editor = new QPlainTextEdit(&dialog);
  editor->setLineWrapMode(QPlainTextEdit::NoWrap);
  editor->setPlainText(settingsJsonFromRoot(captureRoot()));
  editor->moveCursor(QTextCursor::Start);
  layout->addWidget(editor, 1);

  auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, &dialog);
  layout->addWidget(buttons);

  connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
  connect(buttons, &QDialogButtonBox::accepted, &dialog, [&]() {
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(editor->toPlainText().toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
      QMessageBox::warning(
          &dialog,
          tr("Invalid JSON"),
          tr("The settings text is not a valid JSON object:\n%1")
              .arg(parseError.errorString()));
      return;
    }

    const QJsonObject root = document.object();
    applyRoot(root);
    storeRoot(root);
    dialog.accept();

    if (auto *bar = m_mainWindow->statusBar())
      bar->showMessage(tr("JSON settings applied"), 5000);
  });

  dialog.exec();
}

void AppSettingsController::showAboutDialog()
{
  if (!m_mainWindow)
    return;

  QMessageBox::about(
      m_mainWindow,
      tr("About QtRNetAnalyzer"),
      tr("<h3>QtRNetAnalyzer</h3>"
         "<p>Qt6 desktop analyzer for CAN and R-Net traffic with live tables, R-Net decoding, tagging, manual replay and signal plotting.</p>"
         "<p><b>Version:</b> %1<br>"
         "<b>Qt:</b> %2<br>"
         "<b>Settings key:</b> %3<br>"
         "<b>QSettings file:</b><br><code>%4</code></p>"
         "<p>Source: github.com/notwendig/QtRNetAnalyzer</p>")
          .arg(QCoreApplication::applicationVersion(),
               QString::fromLatin1(qVersion()),
               QString::fromLatin1(kSettingsKey),
               m_settings.fileName().toHtmlEscaped()));
}

QJsonObject AppSettingsController::captureRoot() const
{
  QJsonObject root;
  root.insert(QStringLiteral("schema"), kSchemaVersion);
  root.insert(QStringLiteral("application"), QCoreApplication::applicationName());
  root.insert(QStringLiteral("version"), QCoreApplication::applicationVersion());
  root.insert(QStringLiteral("savedAt"), QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs));
  root.insert(QStringLiteral("qtSettingsKey"), QString::fromLatin1(kSettingsKey));
  root.insert(QStringLiteral("qtSettingsFile"), m_settings.fileName());
  root.insert(QStringLiteral("state"), captureState());
  return root;
}

QJsonObject AppSettingsController::captureState() const
{
  QJsonObject state;
  if (!m_mainWindow)
    return state;

  QJsonObject window;
  window.insert(QStringLiteral("geometry"), encodeState(m_mainWindow->saveGeometry()));
  window.insert(QStringLiteral("windowState"), encodeState(m_mainWindow->saveState()));
  window.insert(QStringLiteral("visible"), m_mainWindow->isVisible());
  state.insert(QStringLiteral("mainWindow"), window);

  QJsonObject widgets;
  const auto list = persistableWidgets();
  for (QWidget *widget : list)
    widgets.insert(objectPath(widget), captureWidgetState(widget));
  state.insert(QStringLiteral("widgets"), widgets);

  return state;
}

QJsonObject AppSettingsController::captureWidgetState(QWidget *widget) const
{
  QJsonObject object;
  if (!widget)
    return object;

  object.insert(QStringLiteral("class"), QString::fromLatin1(widget->metaObject()->className()));

  if (auto *checkBox = qobject_cast<QCheckBox *>(widget)) {
    object.insert(QStringLiteral("checked"), checkBox->isChecked());
  } else if (auto *comboBox = qobject_cast<QComboBox *>(widget)) {
    object.insert(QStringLiteral("currentIndex"), comboBox->currentIndex());
    object.insert(QStringLiteral("currentText"), comboBox->currentText());
    object.insert(QStringLiteral("currentData"), QJsonValue::fromVariant(comboBox->currentData()));
  } else if (auto *lineEdit = qobject_cast<QLineEdit *>(widget)) {
    object.insert(QStringLiteral("text"), lineEdit->text());
  } else if (auto *spinBox = qobject_cast<QSpinBox *>(widget)) {
    object.insert(QStringLiteral("value"), spinBox->value());
  } else if (auto *tabWidget = qobject_cast<QTabWidget *>(widget)) {
    object.insert(QStringLiteral("currentIndex"), tabWidget->currentIndex());
  } else if (auto *headerView = qobject_cast<QHeaderView *>(widget)) {
    object.insert(QStringLiteral("state"), encodeState(headerView->saveState()));
  } else if (auto *splitter = qobject_cast<QSplitter *>(widget)) {
    object.insert(QStringLiteral("state"), encodeState(splitter->saveState()));
  }

  return object;
}

QJsonObject AppSettingsController::storedRoot() const
{
  const QString jsonText = m_settings.value(QString::fromLatin1(kSettingsKey)).toString();
  if (jsonText.trimmed().isEmpty())
    return {};

  QJsonParseError parseError;
  const QJsonDocument document = QJsonDocument::fromJson(jsonText.toUtf8(), &parseError);
  if (parseError.error != QJsonParseError::NoError || !document.isObject())
    return {};

  return document.object();
}

void AppSettingsController::applyRoot(const QJsonObject &root)
{
  if (root.isEmpty())
    return;
  applyState(stateFromRoot(root));
}

void AppSettingsController::applyState(const QJsonObject &state)
{
  if (!m_mainWindow)
    return;

  const QJsonObject widgets = state.value(QStringLiteral("widgets")).toObject();
  if (!widgets.isEmpty()) {
    QHash<QString, QWidget *> widgetsByPath;
    for (QWidget *widget : persistableWidgets())
      widgetsByPath.insert(objectPath(widget), widget);

    for (auto it = widgets.constBegin(); it != widgets.constEnd(); ++it) {
      QWidget *widget = widgetsByPath.value(it.key(), nullptr);
      if (widget && it.value().isObject())
        applyWidgetState(widget, it.value().toObject());
    }
  }

  const QJsonObject window = state.value(QStringLiteral("mainWindow")).toObject();
  const QString geometry = window.value(QStringLiteral("geometry")).toString();
  if (!geometry.isEmpty())
    m_mainWindow->restoreGeometry(decodeState(geometry));

  const QString windowState = window.value(QStringLiteral("windowState")).toString();
  if (!windowState.isEmpty())
    m_mainWindow->restoreState(decodeState(windowState));
}

void AppSettingsController::applyWidgetState(QWidget *widget, const QJsonObject &state)
{
  if (!widget || state.isEmpty())
    return;

  if (auto *checkBox = qobject_cast<QCheckBox *>(widget)) {
    if (state.contains(QStringLiteral("checked"))) {
      const QSignalBlocker blocker(checkBox);
      checkBox->setChecked(state.value(QStringLiteral("checked")).toBool(checkBox->isChecked()));
    }
  } else if (auto *comboBox = qobject_cast<QComboBox *>(widget)) {
    const QSignalBlocker blocker(comboBox);
    const QVariant data = state.value(QStringLiteral("currentData")).toVariant();
    int index = data.isValid() ? comboBox->findData(data) : -1;
    if (index < 0) {
      const QString text = state.value(QStringLiteral("currentText")).toString();
      if (!text.isEmpty())
        index = comboBox->findText(text);
    }
    if (index < 0)
      index = state.value(QStringLiteral("currentIndex")).toInt(comboBox->currentIndex());
    if (index >= 0 && index < comboBox->count())
      comboBox->setCurrentIndex(index);
  } else if (auto *lineEdit = qobject_cast<QLineEdit *>(widget)) {
    if (state.contains(QStringLiteral("text"))) {
      const QSignalBlocker blocker(lineEdit);
      lineEdit->setText(state.value(QStringLiteral("text")).toString(lineEdit->text()));
    }
  } else if (auto *spinBox = qobject_cast<QSpinBox *>(widget)) {
    if (state.contains(QStringLiteral("value"))) {
      const QSignalBlocker blocker(spinBox);
      spinBox->setValue(state.value(QStringLiteral("value")).toInt(spinBox->value()));
    }
  } else if (auto *tabWidget = qobject_cast<QTabWidget *>(widget)) {
    const int index = state.value(QStringLiteral("currentIndex")).toInt(tabWidget->currentIndex());
    if (index >= 0 && index < tabWidget->count())
      tabWidget->setCurrentIndex(index);
  } else if (auto *headerView = qobject_cast<QHeaderView *>(widget)) {
    const QString encoded = state.value(QStringLiteral("state")).toString();
    if (!encoded.isEmpty())
      headerView->restoreState(decodeState(encoded));
  } else if (auto *splitter = qobject_cast<QSplitter *>(widget)) {
    const QString encoded = state.value(QStringLiteral("state")).toString();
    if (!encoded.isEmpty())
      splitter->restoreState(decodeState(encoded));
  }
}

void AppSettingsController::storeRoot(const QJsonObject &root)
{
  const QJsonObject state = stateFromRoot(root);
  if (canonicalStateJson(state) == canonicalStateJson(m_defaultState)) {
    m_settings.remove(QString::fromLatin1(kSettingsKey));
  } else {
    m_settings.setValue(QString::fromLatin1(kSettingsKey), settingsJsonFromRoot(root));
  }
  m_settings.sync();
}

QVector<QWidget *> AppSettingsController::persistableWidgets() const
{
  QVector<QWidget *> result;
  if (!m_mainWindow)
    return result;

  const auto children = m_mainWindow->findChildren<QWidget *>();
  result.reserve(children.size());
  for (QWidget *widget : children) {
    if (isPersistableWidget(widget))
      result.push_back(widget);
  }

  std::sort(result.begin(), result.end(), [this](QWidget *left, QWidget *right) {
    return objectPath(left) < objectPath(right);
  });

  return result;
}

QString AppSettingsController::objectPath(const QObject *object) const
{
  if (!object || !m_mainWindow)
    return {};

  QStringList parts;
  const QObject *current = object;
  while (current && current != m_mainWindow) {
    parts.prepend(objectSegment(current));
    current = current->parent();
  }

  parts.prepend(QStringLiteral("MainWindow"));
  return parts.join('/');
}

QString AppSettingsController::objectSegment(const QObject *object) const
{
  if (!object)
    return {};

  const QString className = QString::fromLatin1(object->metaObject()->className());
  const QString objectName = safeName(object->objectName());
  if (!objectName.isEmpty() && !objectName.startsWith(QStringLiteral("qt_")))
    return QStringLiteral("%1(%2)").arg(className, objectName);

  int index = 0;
  const QObject *parent = object->parent();
  if (parent) {
    const auto siblings = parent->children();
    for (QObject *sibling : siblings) {
      if (sibling == object)
        break;
      if (QString::fromLatin1(sibling->metaObject()->className()) == className)
        ++index;
    }
  }

  return QStringLiteral("%1#%2").arg(className).arg(index);
}

bool AppSettingsController::isPersistableWidget(QWidget *widget)
{
  if (!widget)
    return false;

  if (qobject_cast<QMenuBar *>(widget) || qobject_cast<QMenu *>(widget))
    return false;

  if (qobject_cast<QCheckBox *>(widget) ||
      qobject_cast<QComboBox *>(widget) ||
      qobject_cast<QSpinBox *>(widget) ||
      qobject_cast<QTabWidget *>(widget) ||
      qobject_cast<QHeaderView *>(widget) ||
      qobject_cast<QSplitter *>(widget)) {
    return true;
  }

  if (qobject_cast<QLineEdit *>(widget))
    return !hasCompositeEditorAncestor(widget);

  return false;
}

bool AppSettingsController::hasCompositeEditorAncestor(QWidget *widget)
{
  for (QWidget *parent = widget ? widget->parentWidget() : nullptr; parent; parent = parent->parentWidget()) {
    if (qobject_cast<QAbstractSpinBox *>(parent) || qobject_cast<QComboBox *>(parent))
      return true;
  }
  return false;
}

QString AppSettingsController::canonicalStateJson(const QJsonObject &state)
{
  return QString::fromUtf8(QJsonDocument(state).toJson(QJsonDocument::Compact));
}

QString AppSettingsController::settingsJsonFromRoot(const QJsonObject &root)
{
  return QString::fromUtf8(QJsonDocument(root).toJson(QJsonDocument::Indented));
}
