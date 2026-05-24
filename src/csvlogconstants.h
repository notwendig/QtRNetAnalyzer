#pragma once

#include <QDateTime>
#include <QDir>
#include <QString>

namespace CsvLogConstants
{
inline QString deferredLogToken()
{
    return QStringLiteral("memory buffer (save on Stop CSV Log)");
}

inline QString defaultLogPath()
{
    const QString stamp = QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss"));
    return QDir::current().filePath(QStringLiteral("R-Netlog-%1.csv").arg(stamp));
}
} // namespace CsvLogConstants
