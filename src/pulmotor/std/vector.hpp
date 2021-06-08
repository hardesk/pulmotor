#include "../serialize.hpp"

namespace pulmotor
{

template<class Ar, class T, class Al>
void serialize_load(Ar& ar, std::vector<T, Al>& v, unsigned version)
{
	u32 sz;
	v.clear();

	ar | sz;

	if constexpr(wants_construct<Ar, T>::value) {
		v.reserve(sz);
		for (size_t i=0; i<sz; ++i)
			ar | pulmotor::construct<T>( [&v](auto&&... args) { v.emplace_back(args...); });
	} else {
		v.resize(sz);

		//ar | array(v.data(), v.size());
		for (size_t i=0; i<sz; ++i)
			ar | v[i];
	}
}

template<class Ar, class T, class Al>
void serialize_save(Ar& ar, std::vector<T, Al>& v, unsigned version)
{
	u32 sz = v.size();
	ar | sz;

	if constexpr(wants_construct<Ar, T>::value) {
		for (size_t i=0; i<sz; ++i)
			ar | v[i];
	} else {
		//ar | array(v.data(), v.size());
		for (size_t i=0; i<sz; ++i)
			ar | v[i];
	}
}

}
