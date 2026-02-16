#pragma once

#include "CelestialBodyTypes.h"

namespace astro {

class IPlanetEphemerisProvider {
public:
    virtual ~IPlanetEphemerisProvider() = default;
    virtual BodyEquatorial computeBody(BodyId bodyId, double jd_tt) const = 0;
};

class SsdKeplerPlanetProvider final : public IPlanetEphemerisProvider {
public:
    explicit SsdKeplerPlanetProvider(bool enableLightTime = true, int lightTimeIterations = 2);

    BodyEquatorial computeBody(BodyId bodyId, double jd_tt) const override;

private:
    bool m_enableLightTime;
    int m_lightTimeIterations;
};

} // namespace astro
