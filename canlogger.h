#pragma once

#include <QFile>
#include <QMutex>
#include <QString>

#include "canframe.h"

class CanLogger
{
  public:
    bool start(const QString &path, QString *errorMessage);
    void stop();
    bool isActive() const;
    void write(const CanFrame &frame);

  private:
    mutable QMutex m_mutex;
    QFile m_file;
};
