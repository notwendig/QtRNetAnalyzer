#include "translationmanager.h"

#include <QAbstractButton>
#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QComboBox>
#include <QCoreApplication>
#include <QEvent>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QPlainTextEdit>
#include <QLibraryInfo>
#include <QLocale>
#include <QPointer>
#include <QSettings>
#include <QTabWidget>
#include <QTextEdit>
#include <QTranslator>
#include <QVariant>
#include <QWidget>

#include <utility>

namespace {
constexpr auto kLanguageSettingsKey = "translation/language";
constexpr auto kSourceTextProperty = "_qtra_i18n_source_text";
constexpr auto kSourceTitleProperty = "_qtra_i18n_source_title";
constexpr auto kSourceToolTipProperty = "_qtra_i18n_source_tool_tip";
constexpr auto kSourceStatusTipProperty = "_qtra_i18n_source_status_tip";
constexpr auto kSourceWhatsThisProperty = "_qtra_i18n_source_whats_this";
constexpr auto kSourceWindowTitleProperty = "_qtra_i18n_source_window_title";
constexpr auto kSourcePlaceholderProperty = "_qtra_i18n_source_placeholder";
constexpr auto kSourceTabTextsProperty = "_qtra_i18n_source_tab_texts";
constexpr auto kComboSourceRole = Qt::UserRole + 847;

QString plainMenuText(QString text)
{
  text.remove(QLatin1Char('&'));
  return text.trimmed();
}

bool isSettingsMenuName(const QString &text)
{
  const QString plain = plainMenuText(text);
  return plain == QStringLiteral("Settings") || plain == QStringLiteral("Einstellungen");
}
} // namespace

TranslationManager::TranslationManager(QApplication *application, QObject *parent)
  : QObject(parent ? parent : application)
  , m_application(application)
{
}

TranslationManager::~TranslationManager()
{
  if (m_application)
    m_application->removeEventFilter(this);
  if (m_translator && m_application)
    m_application->removeTranslator(m_translator);
  if (m_qtTranslator && m_application)
    m_application->removeTranslator(m_qtTranslator);
  delete m_translator;
  delete m_qtTranslator;
}

void TranslationManager::loadInitialLanguage()
{
  QSettings settings;
  const QString stored = settings.value(QString::fromLatin1(kLanguageSettingsKey), QStringLiteral("system")).toString();
  setLanguage(stored);
}

void TranslationManager::installOn(QMainWindow *mainWindow)
{
  m_mainWindow = mainWindow;
  if (!m_mainWindow || !m_application)
    return;

  m_application->installEventFilter(this);
  createLanguageMenu();
  updateLanguageActions();
  retranslateAll();
}

QString TranslationManager::language() const
{
  return m_languageCode;
}

void TranslationManager::setLanguage(const QString &languageCode)
{
  const QString normalized = normalizedLanguageCode(languageCode);
  m_languageCode = normalized;

  if (m_translator && m_application)
    m_application->removeTranslator(m_translator);
  if (m_qtTranslator && m_application)
    m_application->removeTranslator(m_qtTranslator);
  delete m_translator;
  delete m_qtTranslator;
  m_translator = nullptr;
  m_qtTranslator = nullptr;

  installTranslatorForLanguage(normalized);

  QSettings settings;
  settings.setValue(QString::fromLatin1(kLanguageSettingsKey), normalized);
  settings.sync();

  updateLanguageActions();
  retranslateAll();
}

bool TranslationManager::eventFilter(QObject *watched, QEvent *event)
{
  if (!m_retranslating && event) {
    switch (event->type()) {
    case QEvent::Show:
    case QEvent::ChildAdded:
    case QEvent::ActionChanged:
    case QEvent::LanguageChange:
      retranslateObjectTree(watched);
      break;
    default:
      break;
    }
  }
  return QObject::eventFilter(watched, event);
}

QString TranslationManager::normalizedLanguageCode(const QString &languageCode) const
{
  const QString trimmed = languageCode.trimmed();
  if (trimmed.isEmpty() || trimmed.compare(QStringLiteral("system"), Qt::CaseInsensitive) == 0)
    return QStringLiteral("system");
  if (trimmed.startsWith(QStringLiteral("de"), Qt::CaseInsensitive))
    return QStringLiteral("de_DE");
  return QStringLiteral("en");
}

QString TranslationManager::effectiveLocaleName(const QString &languageCode) const
{
  if (languageCode == QStringLiteral("system"))
    return QLocale::system().name();
  return languageCode;
}

bool TranslationManager::installTranslatorForLanguage(const QString &languageCode)
{
  if (!m_application)
    return false;

  const QString localeName = effectiveLocaleName(languageCode);
  if (!localeName.startsWith(QStringLiteral("de"), Qt::CaseInsensitive))
    return false;

  auto *qtTranslator = new QTranslator(this);
  const QString qtTranslationsPath = QLibraryInfo::path(QLibraryInfo::TranslationsPath);
  if (qtTranslator->load(QStringLiteral("qtbase_de"), qtTranslationsPath)) {
    m_application->installTranslator(qtTranslator);
    m_qtTranslator = qtTranslator;
  } else {
    delete qtTranslator;
  }

  auto *translator = new QTranslator(this);
  if (!translator->load(QStringLiteral(":/i18n/QtRNetAnalyzer_de_DE.qm"))) {
    delete translator;
    return m_qtTranslator != nullptr;
  }

  m_application->installTranslator(translator);
  m_translator = translator;
  return true;
}

void TranslationManager::createLanguageMenu()
{
  if (!m_mainWindow)
    return;

  QMenu *settingsMenu = findOrCreateSettingsMenu();
  if (!settingsMenu)
    return;

  if (m_languageMenu)
    return;

  settingsMenu->addSeparator();
  m_languageMenu = settingsMenu->addMenu(QStringLiteral("&Language"));

  m_languageGroup = new QActionGroup(this);
  m_languageGroup->setExclusive(true);

  m_systemLanguageAction = m_languageMenu->addAction(QStringLiteral("System default"));
  m_systemLanguageAction->setCheckable(true);
  m_systemLanguageAction->setData(QStringLiteral("system"));
  m_languageGroup->addAction(m_systemLanguageAction);

  m_englishLanguageAction = m_languageMenu->addAction(QStringLiteral("English"));
  m_englishLanguageAction->setCheckable(true);
  m_englishLanguageAction->setData(QStringLiteral("en"));
  m_languageGroup->addAction(m_englishLanguageAction);

  m_germanLanguageAction = m_languageMenu->addAction(QStringLiteral("Deutsch"));
  m_germanLanguageAction->setCheckable(true);
  m_germanLanguageAction->setData(QStringLiteral("de_DE"));
  m_languageGroup->addAction(m_germanLanguageAction);

  connect(m_languageGroup, &QActionGroup::triggered, this, [this](QAction *action) {
    if (!action)
      return;
    setLanguage(action->data().toString());
  });
}

void TranslationManager::updateLanguageActions()
{
  if (!m_languageGroup)
    return;

  const auto actions = m_languageGroup->actions();
  for (QAction *action : actions) {
    if (!action)
      continue;
    const bool active = action->data().toString() == m_languageCode;
    action->setChecked(active);
  }
}

QMenu *TranslationManager::findOrCreateSettingsMenu() const
{
  if (!m_mainWindow || !m_mainWindow->menuBar())
    return nullptr;

  QMenuBar *bar = m_mainWindow->menuBar();
  const auto actions = bar->actions();
  for (QAction *action : actions) {
    if (!action || !action->menu())
      continue;
    if (isSettingsMenuName(action->text()) || isSettingsMenuName(action->menu()->title()))
      return action->menu();
  }

  return bar->addMenu(QStringLiteral("&Settings"));
}

void TranslationManager::retranslateAll()
{
  if (!m_application)
    return;

  const bool previous = std::exchange(m_retranslating, true);

  const auto widgets = m_application->topLevelWidgets();
  for (QWidget *widget : widgets)
    retranslateObjectTree(widget);

  m_retranslating = previous;
}

void TranslationManager::retranslateObjectTree(QObject *object)
{
  if (!object)
    return;

  retranslateObject(object);

  const auto children = object->children();
  for (QObject *child : children)
    retranslateObjectTree(child);

  if (auto *widget = qobject_cast<QWidget *>(object)) {
    const auto actions = widget->actions();
    for (QAction *action : actions)
      retranslateAction(action);
  }

  if (auto *menu = qobject_cast<QMenu *>(object)) {
    const auto actions = menu->actions();
    for (QAction *action : actions)
      retranslateAction(action);
  }
}

void TranslationManager::retranslateObject(QObject *object)
{
  if (!object)
    return;

  if (auto *action = qobject_cast<QAction *>(object)) {
    retranslateAction(action);
    return;
  }

  if (auto *widget = qobject_cast<QWidget *>(object))
    retranslateWidget(widget);
}

void TranslationManager::retranslateWidget(QWidget *widget)
{
  if (!widget)
    return;

  const QString windowTitle = widget->windowTitle();
  if (!windowTitle.isEmpty()) {
    const QString source = sourceText(widget, kSourceWindowTitleProperty, windowTitle);
    widget->setWindowTitle(translateUiText(source));
  }

  if (auto *label = qobject_cast<QLabel *>(widget)) {
    const QString current = label->text();
    if (!current.isEmpty()) {
      const QString source = sourceText(label, kSourceTextProperty, current);
      label->setText(translateUiText(source));
    }
  } else if (auto *button = qobject_cast<QAbstractButton *>(widget)) {
    const QString current = button->text();
    if (!current.isEmpty()) {
      const QString source = sourceText(button, kSourceTextProperty, current);
      button->setText(translateUiText(source));
    }
  } else if (auto *groupBox = qobject_cast<QGroupBox *>(widget)) {
    const QString current = groupBox->title();
    if (!current.isEmpty()) {
      const QString source = sourceText(groupBox, kSourceTitleProperty, current);
      groupBox->setTitle(translateUiText(source));
    }
  }

  if (auto *lineEdit = qobject_cast<QLineEdit *>(widget)) {
    const QString current = lineEdit->placeholderText();
    if (!current.isEmpty()) {
      const QString source = sourceText(lineEdit, kSourcePlaceholderProperty, current);
      lineEdit->setPlaceholderText(translateUiText(source));
    }
  } else if (auto *plainTextEdit = qobject_cast<QPlainTextEdit *>(widget)) {
    const QString current = plainTextEdit->placeholderText();
    if (!current.isEmpty()) {
      const QString source = sourceText(plainTextEdit, kSourcePlaceholderProperty, current);
      plainTextEdit->setPlaceholderText(translateUiText(source));
    }
  } else if (auto *textEdit = qobject_cast<QTextEdit *>(widget)) {
    const QString current = textEdit->placeholderText();
    if (!current.isEmpty()) {
      const QString source = sourceText(textEdit, kSourcePlaceholderProperty, current);
      textEdit->setPlaceholderText(translateUiText(source));
    }
  }

  const QString toolTip = widget->toolTip();
  if (!toolTip.isEmpty()) {
    const QString source = sourceText(widget, kSourceToolTipProperty, toolTip);
    widget->setToolTip(translateUiText(source));
  }

  const QString whatsThis = widget->whatsThis();
  if (!whatsThis.isEmpty()) {
    const QString source = sourceText(widget, kSourceWhatsThisProperty, whatsThis);
    widget->setWhatsThis(translateUiText(source));
  }

  if (auto *comboBox = qobject_cast<QComboBox *>(widget))
    retranslateComboBox(comboBox);

  if (auto *tabWidget = qobject_cast<QTabWidget *>(widget)) {
    QStringList sources = tabWidget->property(kSourceTabTextsProperty).toStringList();
    if (sources.size() != tabWidget->count()) {
      sources.clear();
      for (int i = 0; i < tabWidget->count(); ++i)
        sources.push_back(tabWidget->tabText(i));
      tabWidget->setProperty(kSourceTabTextsProperty, sources);
    }

    for (int i = 0; i < tabWidget->count() && i < sources.size(); ++i)
      tabWidget->setTabText(i, translateUiText(sources.at(i)));
  }
}

void TranslationManager::retranslateAction(QAction *action)
{
  if (!action)
    return;

  const QString current = action->text();
  if (!current.isEmpty()) {
    const QString source = sourceText(action, kSourceTextProperty, current);
    action->setText(translateUiText(source));
  }

  const QString toolTip = action->toolTip();
  if (!toolTip.isEmpty()) {
    const QString source = sourceText(action, kSourceToolTipProperty, toolTip);
    action->setToolTip(translateUiText(source));
  }

  const QString statusTip = action->statusTip();
  if (!statusTip.isEmpty()) {
    const QString source = sourceText(action, kSourceStatusTipProperty, statusTip);
    action->setStatusTip(translateUiText(source));
  }

  const QString whatsThis = action->whatsThis();
  if (!whatsThis.isEmpty()) {
    const QString source = sourceText(action, kSourceWhatsThisProperty, whatsThis);
    action->setWhatsThis(translateUiText(source));
  }

}

void TranslationManager::retranslateComboBox(QComboBox *comboBox)
{
  if (!comboBox)
    return;

  for (int i = 0; i < comboBox->count(); ++i) {
    QString source = comboBox->itemData(i, kComboSourceRole).toString();
    const QString current = comboBox->itemText(i);

    if (source.isEmpty()) {
      source = current;
      comboBox->setItemData(i, source, kComboSourceRole);
    }

    comboBox->setItemText(i, translateUiText(source));
  }
}

QString TranslationManager::sourceText(QObject *object, const char *propertyName, const QString &currentText) const
{
  if (!object || !propertyName)
    return currentText;

  const QString existing = object->property(propertyName).toString();
  if (!existing.isEmpty())
    return existing;

  object->setProperty(propertyName, currentText);
  return currentText;
}

QString TranslationManager::translateUiText(const QString &sourceText) const
{
  if (sourceText.isEmpty())
    return sourceText;

  const QByteArray utf8 = sourceText.toUtf8();
  return QCoreApplication::translate("UiText", utf8.constData());
}
