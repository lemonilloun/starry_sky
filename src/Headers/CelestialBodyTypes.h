#pragma once

#include <cstdint>
#include <string>

namespace astro {

enum class BodyId : uint64_t {
    Sun     = 1010101010ULL,
    Moon    = 1010101011ULL,
    Mercury = 1010101012ULL,
    Venus   = 1010101013ULL,
    Mars    = 1010101014ULL,
    Jupiter = 1010101015ULL,
    Saturn  = 1010101016ULL,
    Uranus  = 1010101017ULL,
    Neptune = 1010101018ULL
};

struct BodyEquatorial {
    double raRad = 0.0;
    double decRad = 0.0;
    double distanceAu = 0.0;
    double magnitude = 99.0;
    double angularDiameterRad = 0.0;
    double illumination = -1.0; // 0..1 for phase-dependent bodies, -1 when not used
    BodyId bodyId = BodyId::Sun;
    std::string name;
};

constexpr uint64_t bodyIdValue(BodyId id)
{
    return static_cast<uint64_t>(id);
}

bool isKnownBodyId(uint64_t rawId);
std::string bodyTypeLabel(BodyId id);

} // namespace astro
