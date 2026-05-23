#pragma once

#include "canframe.h"

#include <QString>
#include <QVector>

class RNetWheelchairSimulator final
{
public:
    enum class Scenario
    {
        JsmLoginDrive
    };

    static QVector<CanFrame> createScenario(Scenario scenario = Scenario::JsmLoginDrive);
    static QString scenarioName(Scenario scenario = Scenario::JsmLoginDrive);

private:
    static CanFrame makeFrame(quint32 timestampUs,
                              quint32 id,
                              const QByteArray &data,
                              direction_t direction,
                              int channel = 0,
                              bool extended = true);
};
