#ifndef PULMOTOR_STD_MAP_HPP_
#define PULMOTOR_STD_MAP_HPP_

#include "../serialize.hpp"
#include "utility.hpp"

namespace pulmotor {

template<class Ar, class K, class T, class C, class Al>
void serialize(Ar& ar, std::map<K, T, C, Al>& m, unsigned version) {

	using map_t = std::map<K, T, C, Al>;
	u32 sz;

	if constexpr(Ar::is_reading)
		m.clear();
	else
		sz = m.size();

	ar | sz;

	if constexpr(Ar::is_reading) {
		for (size_t i=0; i<sz; ++i) {
			if constexpr(pulmotor::wants_construct<Ar, T>::value) {
				typename map_t::key_type k;
				ar | k;
				ar | pulmotor::construct<T>( [&m, &k](auto&&... args) { m.emplace(k, args...); });
			} else {
				typename map_t::value_type v;
				ar | v;
				m.insert(v);
			}
		}
	} else {
		for (auto it=m.begin(); it != m.end(); ++it) {
			if constexpr(pulmotor::wants_construct<Ar, T>::value) {
				ar | it->first;
				ar | it->second;
			} else {
				ar | *it;
			}
		}
	}

}

} // pulmotor

#endif // PULMOTOR_STD_MAP_HPP_
