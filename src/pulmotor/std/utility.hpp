#ifndef PULMOTOR_UTILITY_HPP_
#define PULMOTOR_UTILITY_HPP_

#include "../serialize.hpp"
#include <utility>

namespace pulmotor
{

template<class F, class S> struct class_version<std::pair<F, S>> { enum { value = pulmotor::no_version }; };

template<class Ar, class F, class S>
inline void serialize(Ar& ar, std::pair<F, S>& p)
{
    ar | p.first | p.second;
}

/*template<class Ar, class... T>
inline void serialize(Ar& ar, std::tuple<T...>& t)
{
    std::apply( [&ar](T const&... args) { ar | ... | args; }, t);
}

template<class Ar, class T>
void serialize(Ar& ar, std::optional<T>& o)
{
    if constexpr(Ar::is_writing) {
        ar | (u8)o.has_value();
        if (o.has_value()) {
            ar | *o;
        }
    } else if constexpr(Ar::is_reading) {
        u8 hv;
        ar | hv;
        if (hv) {
            if constexpr(pulmotor::wants_construct<T>::value) {
                ar | pulmotor::construct<T>([&o](auto&&... args) { o.emplace(std::forward<decltype(args)>(args)...); });
            } else {
                o = T();
                ar | o.value();
            }
        }
    }
}*/

}

#endif
