#ifndef PULMOTOR_STD_MEMORY_HPP_
#define PULMOTOR_STD_MEMORY_HPP_

#include <memory>

#include "../serialize.hpp"
#include "utility.hpp"

namespace pulmotor {

template<class Ar, class T, class D>
inline void serialize_load(Ar& ar, std::unique_ptr<T, D>& p)
{
    using Tb = typename std::remove_const<T>::type;
    Tb* r = nullptr;
    ar | r;
    p.reset(r);
}

template<class Ar, class T, class D>
inline void serialize_save(Ar& ar, std::unique_ptr<T, D> const& p)
{
    T const* r = p.get();
    ar | r;
}

}

#endif // PULMOTOR_STD_MEMORY_HPP_
