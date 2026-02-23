#pragma once

#include "MoonEphemeris.h"

#include <string>

namespace astro {

class Elp82bMoonProvider final : public IMoonEphemerisProvider {
public:
    explicit Elp82bMoonProvider(std::string dataDirectory = "src/data/elp82b", double truncationPrecisionRad = 0.0);

    bool isReady() const;
    const std::string& lastError() const;

    BodyEquatorial computeMoon(double jd_tt, const MoonObserverEqDate* observerEqDate = nullptr) const override;

private:
    bool ensureLoaded() const;

    std::string m_dataDirectory;
    double m_truncationPrecisionRad;

    mutable bool m_loaded = false;
    mutable bool m_ready = false;
    mutable std::string m_lastError;
};

} // namespace astro
