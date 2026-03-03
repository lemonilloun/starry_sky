#pragma once

#include <QDate>
#include "CelestialBodyTypes.h"

struct SimulationRequest {
    astro::BodyId body = astro::BodyId::Moon;
    QDate startDate;
    QDate endDate;
    int stepDays = 1;
    int playbackMsPerFrame = 40;
};
