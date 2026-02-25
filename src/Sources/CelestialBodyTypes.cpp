#include "CelestialBodyTypes.h"

namespace astro {

bool isKnownBodyId(uint64_t rawId)
{
    return rawId == bodyIdValue(BodyId::Sun)
        || rawId == bodyIdValue(BodyId::Moon)
        || rawId == bodyIdValue(BodyId::Mercury)
        || rawId == bodyIdValue(BodyId::Venus)
        || rawId == bodyIdValue(BodyId::Mars)
        || rawId == bodyIdValue(BodyId::Jupiter)
        || rawId == bodyIdValue(BodyId::Saturn)
        || rawId == bodyIdValue(BodyId::Uranus)
        || rawId == bodyIdValue(BodyId::Neptune);
}

std::string bodyTypeLabel(BodyId id)
{
    switch (id) {
    case BodyId::Sun:
        return "Star";
    case BodyId::Moon:
        return "Moon";
    case BodyId::Mercury:
    case BodyId::Venus:
    case BodyId::Mars:
    case BodyId::Jupiter:
    case BodyId::Saturn:
    case BodyId::Uranus:
    case BodyId::Neptune:
        return "Planet";
    }
    return "Body";
}

} // namespace astro
