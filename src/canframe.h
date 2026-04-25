#pragma once

#include <QByteArray>
#include <QString>
#include <QtGlobal>

typedef enum {dir_rx,dir_tx} direction_t;

struct CanFrame
{
    quint32 id = 0;
    QByteArray data;
    bool extended = true;
    bool remote = false;
    bool error = false;
    quint32 hwTimestamp = 0;
    QString hostTime;

    int channel=-1;
    direction_t direction=dir_tx;
};
