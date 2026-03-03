#pragma once

#include "CelestialBodyTypes.h"

namespace astro {

struct MoonObserverEqDate {
    double xKm = 0.0;
    double yKm = 0.0;
    double zKm = 0.0;
};

class IMoonEphemerisProvider {
public:
    virtual ~IMoonEphemerisProvider() = default;
    virtual BodyEquatorial computeMoon(double jd_tt, const MoonObserverEqDate* observerEqDate = nullptr) const = 0;
};

class MoonLiteProvider final : public IMoonEphemerisProvider {
public:
    BodyEquatorial computeMoon(double jd_tt, const MoonObserverEqDate* observerEqDate = nullptr) const override;
};

} // namespace astro
