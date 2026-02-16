#pragma once

#include "CelestialBodyTypes.h"

namespace astro {

class IMoonEphemerisProvider {
public:
    virtual ~IMoonEphemerisProvider() = default;
    virtual BodyEquatorial computeMoon(double jd_tt) const = 0;
};

class MoonLiteProvider final : public IMoonEphemerisProvider {
public:
    BodyEquatorial computeMoon(double jd_tt) const override;
};

} // namespace astro
