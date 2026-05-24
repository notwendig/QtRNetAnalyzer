#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>

#include "appsettingscontroller.h"
#include "mainwindow.h"
#include "translationmanager.h"

int main(int argc, char *argv[])
{
  QApplication app(argc, argv);

  QCoreApplication::setOrganizationName(QStringLiteral("notwendig"));
  QCoreApplication::setOrganizationDomain(QStringLiteral("github.com/notwendig"));
  QCoreApplication::setApplicationName(QStringLiteral("QtRNetAnalyzer"));
  QCoreApplication::setApplicationVersion(QStringLiteral("1.1.0"));

  TranslationManager translations(&app);
  translations.loadInitialLanguage();

  QCommandLineParser parser;
  parser.setApplicationDescription(
      QCoreApplication::translate("UiText", "Qt6 desktop analyzer for CAN and R-Net traffic with live CAN and manual simulation replay"));
  parser.addHelpOption();
  parser.addVersionOption();

  QCommandLineOption inputOption(
      QStringList() << QStringLiteral("i") << QStringLiteral("input"),
      QCoreApplication::translate("UiText", "Select a candump text file as simulation source without auto-start."),
      QCoreApplication::translate("UiText", "file"));
  parser.addOption(inputOption);

  parser.process(app);

  MainWindow window(parser.value(inputOption));
  AppSettingsController settings(&window);
  translations.installOn(&window);

  window.show();
  return app.exec();
}
