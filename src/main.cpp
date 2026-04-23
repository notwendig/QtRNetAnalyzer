// SPDX-License-Identifier: GPL-3.0-only
/*
 * QtRNetAnalyzer
 *
 * Copyright (c) 2026
 * ChatGPT (GPT-5.4 Thinking)
 * Jürgen Willi Sievers <JSievers@NadiSoft.de>
 */
#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>

#include "mainwindow.h"

int main(int argc, char *argv[]) {

    QApplication app(argc, argv);

    QCoreApplication::setApplicationName(QStringLiteral("QtRNetAnalyzer"));
    QCoreApplication::setApplicationVersion(QStringLiteral("1.0"));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("QtRNetAnalyzer with live CAN and candump replay"));
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption inputOption(
        QStringList() << QStringLiteral("i") << QStringLiteral("input"),
        QStringLiteral("Read CAN frames from a candump text file instead of live CAN."),
        QStringLiteral("file"));
    parser.addOption(inputOption);

    parser.process(app);

    MainWindow w(parser.value(inputOption));
    w.show();
    return app.exec();
}
