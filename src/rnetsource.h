#pragma once

#include "canframe.h"

#include <QObject>

class RNetSource : public QObject
{
    Q_OBJECT
public:
    using QObject::QObject;
    ~RNetSource() override = default;

public slots:
    virtual void start() = 0;
    virtual void stop() = 0;

signals:
    void frameReceived(const CanFrame &frame);
};
