#pragma once

#include <QFile>
#include <QMutex>
#include <QString>
#include <QVector>

#include "canframe.h"

class CanLogger
{
public:
    bool start(const QString &path, QString *errorMessage);
    void stop();
    bool isActive() const;
    void write(const CanFrame &frame);
    qsizetype bufferedFrameCount() const;

private:
    enum class Mode
    {
        Inactive,
        ImmediateFile,
        DeferredMemory
    };

    static void writeHeader(QTextStream &out);
    static void writeFrame(QTextStream &out, const CanFrame &frame);

    mutable QMutex m_mutex;
    QFile m_file;
    QVector<CanFrame> m_deferredFrames;
    Mode m_mode = Mode::Inactive;
};
