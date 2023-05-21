#ifndef PULMOTOR_STD_STRING_HPP_
#define PULMOTOR_STD_STRING_HPP_

#include "../serialize.hpp"
#include <string>

namespace pulmotor
{

template<class Ch, class Tr, class Al>
struct class_version<std::basic_string<Ch, Tr, Al>> { static unsigned const value = pulmotor::no_prefix; };

// template<class Ar, class Ch, class Tr, class Al>
//     // requires (is_text_archive<Ar>::value && is_save_archive<Ar>::value)
// void serialize_save(Ar& ar, std::basic_string<Ch, Tr, Al>& s)
// {
//     ar.write_string(s);
// }

// template<class Ar, class Ch, class Tr, class Al>
// void serialize_save(Ar& ar, std::basic_string<Ch, Tr, Al>& s)
// {
//     ar.read_string(s);
// }

template<class Ar, class Ch, class Tr, class Al>
void serialize(Ar& ar, std::basic_string<Ch, Tr, Al>& s, unsigned version)
{
    size_t sz = s.size();
    ar | array_size(sz);
    if constexpr (Ar::is_reading)
        s.resize(sz);
    ar | array_data(s.data(), sz);
}

// template<class Ar, class Ch, class Tr, class Al>
// void serialize_save(Ar& ar, std::basic_string<Ch, Tr, Al>& s, unsigned version)
// {
//     #if 1
//     ar | array(s.data(), s.size());
//     #else
//     size_t sz = s.size();
//     ar | array_size(sz);
//     if (sz) ar | array_data(s.data(), sz);
//     #endif
// }

// template<class Ar, class Ch, class Tr, class Al>
// void serialize_load(Ar& ar, std::basic_string<Ch, Tr, Al>& s, unsigned version)
// {
//     size_t sz;
//     ar | pulsz;

//     if (sz) {
//         s.resize(sz);
//         ar | array(s.data(), sz);
//     } else {
//         s.clear();
//         // s.shrink_to_fit();
//     }
// }

}

#endif // PULMOTOR_STD_STRING_HPP_
