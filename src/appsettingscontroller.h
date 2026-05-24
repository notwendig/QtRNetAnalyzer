#pragma once

#include <QJsonObject>
#include <QObject>
#include <QSettings>
#include <QString>
#include <QVector>

class QMainWindow;
class QWidget;

class AppSettingsController final : public QObject
{
  Q_OBJECT

public:
  explicit AppSettingsController(QMainWindow *mainWindow);
  ~AppSettingsController() override;

  void restore();
  void save();
  void resetToDefaults();
  void editJsonSettings();
  void showAboutDialog();

private:
  QJsonObject captureRoot() const;
  QJsonObject captureState() const;
  QJsonObject captureWidgetState(QWidget *widget) const;
  QJsonObject storedRoot() const;

  void applyRoot(const QJsonObject &root);
  void applyState(const QJsonObject &state);
  void applyWidgetState(QWidget *widget, const QJsonObject &state);
  void storeRoot(const QJsonObject &root);
  void installMenus();

  QVector<QWidget *> persistableWidgets() const;
  QString objectPath(const QObject *object) const;
  QString objectSegment(const QObject *object) const;

  static bool isPersistableWidget(QWidget *widget);
  static bool hasCompositeEditorAncestor(QWidget *widget);
  static QString canonicalStateJson(const QJsonObject &state);
  static QString settingsJsonFromRoot(const QJsonObject &root);

  QMainWindow *m_mainWindow = nullptr;
  QSettings m_settings;
  QJsonObject m_defaultState;
  bool m_savedDuringShutdown = false;
};
