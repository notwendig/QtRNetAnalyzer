#pragma once

#include <QFileDialog>
#include <QString>
#include <QStringList>
#include <QWidget>

#include "csvlogconstants.h"

class QtraDeferredCsvFileDialog final
{
public:
    static QString getSaveFileName(QWidget *parent = nullptr,
                                   const QString &caption = QString(),
                                   const QString &dir = QString(),
                                   const QString &filter = QString(),
                                   QString *selectedFilter = nullptr,
                                   QFileDialog::Options options = QFileDialog::Options())
    {
        Q_UNUSED(parent)
        Q_UNUSED(dir)
        Q_UNUSED(selectedFilter)
        Q_UNUSED(options)

        if (caption.contains(QStringLiteral("CSV"), Qt::CaseInsensitive) ||
            filter.contains(QStringLiteral("*.csv"), Qt::CaseInsensitive)) {
            return CsvLogConstants::deferredLogToken();
        }

        return ::QFileDialog::getSaveFileName(parent, caption, dir, filter, selectedFilter, options);
    }

    static QString getOpenFileName(QWidget *parent = nullptr,
                                   const QString &caption = QString(),
                                   const QString &dir = QString(),
                                   const QString &filter = QString(),
                                   QString *selectedFilter = nullptr,
                                   QFileDialog::Options options = QFileDialog::Options())
    {
        return ::QFileDialog::getOpenFileName(parent, caption, dir, filter, selectedFilter, options);
    }

    static QStringList getOpenFileNames(QWidget *parent = nullptr,
                                        const QString &caption = QString(),
                                        const QString &dir = QString(),
                                        const QString &filter = QString(),
                                        QString *selectedFilter = nullptr,
                                        QFileDialog::Options options = QFileDialog::Options())
    {
        return ::QFileDialog::getOpenFileNames(parent, caption, dir, filter, selectedFilter, options);
    }
};
