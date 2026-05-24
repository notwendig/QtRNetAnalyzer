#include "canlogger.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QMutexLocker>
#include <QSaveFile>
#include <QTextStream>

#include "csvlogconstants.h"

bool CanLogger::start(const QString &path, QString *errorMessage)
{
    QMutexLocker locker(&m_mutex);

    if (m_mode != Mode::Inactive || m_file.isOpen()) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Logger already active");
        return false;
    }

    if (path == CsvLogConstants::deferredLogToken()) {
        m_deferredFrames.clear();
        m_mode = Mode::DeferredMemory;
        return true;
    }

    m_file.setFileName(path);
    if (!m_file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Cannot open log file: %1").arg(path);
        return false;
    }

    QTextStream out(&m_file);
    writeHeader(out);
    out.flush();
    m_mode = Mode::ImmediateFile;
    return true;
}

void CanLogger::stop()
{
    QVector<CanFrame> framesToSave;
    bool deferred = false;

    {
        QMutexLocker locker(&m_mutex);

        if (m_mode == Mode::ImmediateFile) {
            if (m_file.isOpen())
                m_file.close();
            m_mode = Mode::Inactive;
            return;
        }

        if (m_mode == Mode::DeferredMemory) {
            framesToSave = m_deferredFrames;
            m_deferredFrames.clear();
            m_mode = Mode::Inactive;
            deferred = true;
        }
    }

    if (!deferred)
        return;

    const QString path = QFileDialog::getSaveFileName(nullptr,
                                                      QStringLiteral("Save CSV log"),
                                                      CsvLogConstants::defaultLogPath(),
                                                      QStringLiteral("CSV Files (*.csv)"));
    if (path.isEmpty())
        return;

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(nullptr,
                             QStringLiteral("CSV Logger"),
                             QStringLiteral("Cannot open log file: %1").arg(path));
        return;
    }

    QTextStream out(&file);
    writeHeader(out);
    for (const CanFrame &frame : framesToSave)
        writeFrame(out, frame);
    out.flush();

    if (!file.commit()) {
        QMessageBox::warning(nullptr,
                             QStringLiteral("CSV Logger"),
                             QStringLiteral("Cannot write log file: %1").arg(path));
    }
}

bool CanLogger::isActive() const
{
    QMutexLocker locker(&m_mutex);
    return m_mode != Mode::Inactive;
}

void CanLogger::write(const CanFrame &frame)
{
    QMutexLocker locker(&m_mutex);

    if (m_mode == Mode::DeferredMemory) {
        m_deferredFrames.push_back(frame);
        return;
    }

    if (m_mode != Mode::ImmediateFile || !m_file.isOpen())
        return;

    QTextStream out(&m_file);
    writeFrame(out, frame);
    out.flush();
}

qsizetype CanLogger::bufferedFrameCount() const
{
    QMutexLocker locker(&m_mutex);
    return m_deferredFrames.size();
}

void CanLogger::writeHeader(QTextStream &out)
{
    out << "host_time,hw_timestamp,id_hex,frame_type,dlc,error,data_hex\n";
}

void CanLogger::writeFrame(QTextStream &out, const CanFrame &frame)
{
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
}
