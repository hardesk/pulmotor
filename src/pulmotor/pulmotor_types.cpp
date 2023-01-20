#include "pulmotor_types.hpp"

namespace pulmotor
{

// https://www.pcg-random.org/posts/bounded-rands.html
template<class Rand>
uint32_t rand_range_impl(Rand& rng, uint32_t range) {
    uint32_t x = rng();
    uint64_t m = uint64_t(x) * uint64_t(range);
    uint32_t l = uint32_t(m);
    if (l < range) {
        uint32_t t = -range;
        if (t >= range) {
            t -= range;
            if (t >= range)
                t %= range;
        }
        while (l < t) {
            x = rng();
            m = uint64_t(x) * uint64_t(range);
            l = uint32_t(m);
        }
    }
    return m >> 32;
}

unsigned rand_range(romu3& rnd, unsigned range)
{
    return rand_range_impl(rnd, range);
}

unsigned rand_range(romu_q32& rnd, unsigned range)
{
    return rand_range_impl(rnd, range);
}

} // pulmotor
