#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>

#include "mainwindow.h"

int main(int argc, char *argv[]) {

    QApplication app(argc, argv);

    QCoreApplication::setApplicationName(QStringLiteral("Qt6 ControlCAN Analyzer Pro"));
    QCoreApplication::setApplicationVersion(QStringLiteral("1.0"));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Qt6 ControlCAN Analyzer Pro with live CAN and manual simulation replay"));
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption inputOption(
        QStringList() << QStringLiteral("i") << QStringLiteral("input"),
        QStringLiteral("Select a candump text file as simulation source without auto-start."),
        QStringLiteral("file"));
    parser.addOption(inputOption);

    parser.process(app);

    MainWindow w(parser.value(inputOption));
    w.show();
    return app.exec();
}
