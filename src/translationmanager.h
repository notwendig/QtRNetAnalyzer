#pragma once

#include <QObject>
#include <QString>

class QAction;
class QActionGroup;
class QApplication;
class QComboBox;
class QMainWindow;
class QMenu;
class QTranslator;
class QWidget;
class QObject;

class TranslationManager final : public QObject
{
  Q_OBJECT

public:
  explicit TranslationManager(QApplication *application, QObject *parent = nullptr);
  ~TranslationManager() override;

  void loadInitialLanguage();
  void installOn(QMainWindow *mainWindow);
  QString language() const;

public slots:
  void setLanguage(const QString &languageCode);

protected:
  bool eventFilter(QObject *watched, QEvent *event) override;

private:
  QString normalizedLanguageCode(const QString &languageCode) const;
  QString effectiveLocaleName(const QString &languageCode) const;
  bool installTranslatorForLanguage(const QString &languageCode);
  void createLanguageMenu();
  void updateLanguageActions();
  QMenu *findOrCreateSettingsMenu() const;
  void retranslateAll();
  void retranslateObjectTree(QObject *object);
  void retranslateObject(QObject *object);
  void retranslateWidget(QWidget *widget);
  void retranslateAction(QAction *action);
  void retranslateComboBox(QComboBox *comboBox);
  QString sourceText(QObject *object, const char *propertyName, const QString &currentText) const;
  QString translateUiText(const QString &sourceText) const;

  QApplication *m_application = nullptr;
  QMainWindow *m_mainWindow = nullptr;
  QTranslator *m_translator = nullptr;
  QTranslator *m_qtTranslator = nullptr;
  QString m_languageCode;
  QMenu *m_languageMenu = nullptr;
  QActionGroup *m_languageGroup = nullptr;
  QAction *m_systemLanguageAction = nullptr;
  QAction *m_englishLanguageAction = nullptr;
  QAction *m_germanLanguageAction = nullptr;
  bool m_retranslating = false;
};
