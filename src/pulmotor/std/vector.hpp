#include "../serialize.hpp"

namespace pulmotor
{

template<class T, class A>
struct class_version<std::vector<T, A>> { static unsigned const value = pulmotor::no_prefix; };

template<class Ar, class T, class Al>
void serialize(Ar& ar, std::vector<T, Al>& v, unsigned version)
{
    size_t sz = v.size();
    ar | pulmotor::array_size(sz);

    if constexpr(wants_construct<Ar, T>::value) {
        // v.clear();
        // v.reserve(sz);
        // for (size_t i=0; i<sz; ++i)
        //     ar | pulmotor::construct<T>( [&v](auto&&... args) { v.emplace_back(args...); });
        // ar.end_array();
    } else {
        if constexpr (Ar::is_reading)
            v.resize(sz);
        ar | pulmotor::array_data(v.data(), sz);
    }
}

// template<class Ar, class T, class Al>
// void serialize_load(Ar& ar, std::vector<T, Al>& v, unsigned version)
// {
//     // if (auto ah = ar | array_helper(v.size())) {
//     //     if constexpr(wants_construct<Ar, T>::value) {
//     //         v.reserve(ah.size());
//     //         for (size_t i=0; i<sz; ++i)
//     //             ah | pulmotor::construct<T>( [&v](auto&&... args) { v.emplace_back(args...); });
//     //     } else {
//     //         v.resize(ah.size());
//     //         for (size_t i=0; i<sz; ++i)
//     //             ar | v[i];
//     //     }
//     // }

//     size_t sz;
//     v.clear();
//     ar.begin_array(sz);

//     if constexpr(wants_construct<Ar, T>::value) {
//         v.reserve(sz);
//         for (size_t i=0; i<sz; ++i)
//             ar | pulmotor::construct<T>( [&v](auto&&... args) { v.emplace_back(args...); });
//     } else {
//         v.resize(sz);

//         //ar | array(v.data(), v.size());
//         for (size_t i=0; i<sz; ++i)
//             ar | v[i];
//     }
//     ar.end_array();
// }

// template<class Ar, class T, class Al>
// void serialize_save(Ar& ar, std::vector<T, Al>& v, unsigned version)
// {
//     array_helper ah (ar);
//     size_t sz = ah.get_size();

//     u32 sz = v.size();
//     ar.begin_array(sz);

//     if constexpr(wants_construct<Ar, T>::value) {
//         for (size_t i=0; i<sz; ++i)
//             ar | v[i];
//     } else {
//         //ar | array(v.data(), v.size());
//         for (size_t i=0; i<sz; ++i)
//             ar | v[i];
//     }
//     ar.end_array();
// }

}
