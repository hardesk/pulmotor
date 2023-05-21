#ifndef PULMOTOR_COMMON_FORMATTER_HPP_
#define PULMOTOR_COMMON_FORMATTER_HPP_

#include <doctest.h>
#include <string>
#include <vector>

inline std::vector<char> operator"" _v(char const* s, size_t l) { return std::vector<char>(s, s + l); }

namespace std {

template<class T>
std::string toStringValue(T const& a)
{
    std::string s;
    if constexpr(std::is_same<T, std::string>::value) {
        return a;
    } else if constexpr(std::is_same<T, char>::value) {
        s += '\'';
        if (a < 32) s += '\\' + std::to_string((int)a); else s += a;
        s += '\'';
    } else if constexpr(std::is_pointer<T>::value && std::is_arithmetic<typename std::remove_pointer<T>::type>::value) {
        s += '&';
        s += std::to_string(*a);
    } else if constexpr(std::is_arithmetic<T>::value) {
        s += std::to_string(a);
    } else {
        if constexpr(std::is_pointer<T>::value) {
            s += '&';
            s += toString(*a).c_str();
        } else
            s += toString(a).c_str();
        //s += '?';
    }
    return s;
}

template<class T, class A>
doctest::String toString(std::vector<T, A> const& v) {
    std::string s;
    s.reserve(v.size() * sizeof(T)*7/2 + 5);
    for(size_t i=0; i<v.size(); ++i) {
        if (v.size() > 32 && (i>=24 && i<v.size()-4)) {
            if (i == 24)
                s += ", ...";
        } else {
            s += toStringValue(v[i]);
            if (i != v.size() - 1)
                s += ", ";
        }
    }
    return s.c_str();
}

template<class K, class T, class C, class A>
doctest::String toString(std::map<K, T, C, A> const& m) {
    std::string s;
    s.reserve(m.size() * sizeof(T)*7/2 * sizeof(K)*7/2 + 5);
    size_t i = 0;
    for(auto it = m.begin(); it != m.end(); ++it, ++i) {
        if (m.size() > 32 && (i>=24 && i<m.size()-4)) {
            if (i == 24)
                s += ", ...";
        } else {
            s += '(' + toStringValue(it->first) + ':' + toStringValue(it->second) + ')';
            if (i != m.size() - 1)
                s += ", ";
        }
    }
    return s.c_str();
}

} // std

#endif // PULMOTOR_COMMON_FORMATTER_HPP_
