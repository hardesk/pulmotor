#ifndef PULMOTOR_STD_STRING_HPP_
#define PULMOTOR_STD_STRING_HPP_

#include "../serialize.hpp"
#include <string>

namespace pulmotor
{

template<class Ch, class Tr, class Al> struct class_version<std::basic_string<Ch, Tr, Al>> { static unsigned const value = pulmotor::no_version; };

template<class Ar, class Ch, class Tr, class Al>
void serialize_save(Ar& ar, std::basic_string<Ch, Tr, Al>& s, unsigned version)
{
	u32 sz = s.size();
	ar | sz;
	if (sz)
		ar | array(s.data(), sz);
}

template<class Ar, class Ch, class Tr, class Al>
void serialize_load(Ar& ar, std::basic_string<Ch, Tr, Al>& s, unsigned version)
{
	u32 sz;
	ar | sz;

	if (sz) {
		s.resize(sz);
		ar | array(s.data(), sz);
	} else {
		s.clear();
		s.shrink_to_fit();
	}
}

}

#endif // PULMOTOR_STD_STRING_HPP_
