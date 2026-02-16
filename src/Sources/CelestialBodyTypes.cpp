#include "CelestialBodyTypes.h"

namespace astro {

bool isKnownBodyId(uint64_t rawId)
{
    return rawId == bodyIdValue(BodyId::Sun)
        || rawId == bodyIdValue(BodyId::Moon)
        || rawId == bodyIdValue(BodyId::Venus)
        || rawId == bodyIdValue(BodyId::Mars);
}

std::string bodyTypeLabel(BodyId id)
{
    switch (id) {
    case BodyId::Sun:
        return "Star";
    case BodyId::Moon:
        return "Moon";
    case BodyId::Venus:
    case BodyId::Mars:
        return "Planet";
    }
    return "Body";
}

} // namespace astro
