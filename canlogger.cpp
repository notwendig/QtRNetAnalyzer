#include "canlogger.h"

#include <QMutexLocker>
#include <QTextStream>

bool CanLogger::start(const QString &path, QString *errorMessage)
{
    QMutexLocker locker(&m_mutex);

    if (m_file.isOpen())
    {
        if (errorMessage)
            *errorMessage = QStringLiteral("Logger already active");
        return false;
    }

    m_file.setFileName(path);
    if (!m_file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        if (errorMessage)
            *errorMessage = QStringLiteral("Cannot open log file: %1").arg(path);
        return false;
    }

    QTextStream out(&m_file);
    out << "host_time,hw_timestamp,id_hex,frame_type,dlc,error,data_hex\n";
    out.flush();

    return true;
}

void CanLogger::stop()
{
    QMutexLocker locker(&m_mutex);

    if (m_file.isOpen())
        m_file.close();
}

bool CanLogger::isActive() const
{
    QMutexLocker locker(&m_mutex);
    return m_file.isOpen();
}

void CanLogger::write(const CanFrame &frame)
{
    QMutexLocker locker(&m_mutex);

    if (!m_file.isOpen())
        return;

    QTextStream out(&m_file);
    out << frame.hostTime << ','
        << frame.hwTimestamp << ','
        << QStringLiteral("0x%1")
               .arg(frame.id, frame.extended ? 8 : 3, 16, QChar('0'))
               .toUpper()
        << ','
        << (frame.extended ? "EXT" : "STD")
        << '/'
        << (frame.remote ? "RTR" : "DATA")
        << ','
        << frame.data.size()
        << ','
        << (frame.error ? "1" : "0")
        << ','
        << frame.data.toHex(' ').toUpper()
        << '\n';

    out.flush();
}
