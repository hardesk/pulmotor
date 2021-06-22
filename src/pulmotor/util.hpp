#ifndef PULMOTOR_UTILS_HPP_
#define PULMOTOR_UTILS_HPP_

#include "pulmotor_config.hpp"
#include "pulmotor_types.hpp"
#include <cstring>
#include <vector>
#include <algorithm>
#include <utility>
#include <stdint.h>
#include <cstdio>
#include <cassert>
#include <system_error>

namespace pulmotor {
namespace util {

PULMOTOR_ATTR_DLL int get_pagesize();
PULMOTOR_ATTR_DLL std::string abi_demangle(char const* name);

template<class T>
inline std::string readable_name ()
{
	typedef typename std::remove_cv<T>::type clean_t;
	return abi_demangle (typeid(clean_t).name());
}

template<class T>
struct short_type_name { };

#define PULMOTOR_BN(x) template<> struct short_type_name<x> { static char const* name; };

PULMOTOR_BN(char)
PULMOTOR_BN(unsigned char)
PULMOTOR_BN(short)
PULMOTOR_BN(unsigned short)
PULMOTOR_BN(int)
PULMOTOR_BN(unsigned)
PULMOTOR_BN(long)
PULMOTOR_BN(unsigned long)
PULMOTOR_BN(long long)
PULMOTOR_BN(unsigned long long)
PULMOTOR_BN(float)
PULMOTOR_BN(double)
PULMOTOR_BN(long double)
PULMOTOR_BN(char16_t)
PULMOTOR_BN(char32_t)


template<class T>
inline typename std::enable_if<std::is_unsigned<T>::value, T>::type
align(T offset, size_t al)
{
	T mask = al-1U;
	return ((offset-1U) | mask) + 1U;
}

template<int Alignment, class T>
inline typename std::enable_if<std::is_unsigned<T>::value, T>::type
align (T off)
{
	assert (Alignment > 0 && (Alignment & (Alignment-1)) == 0);
	return (off-1 | Alignment-1) + 1;
}

template<class T>
inline typename std::enable_if<std::is_integral<T>::value, bool>::type
is_aligned(T offset, size_t align)
{
	return (offset & (align-1U)) == 0;
}

inline size_t pad(size_t offset, size_t align)
{
	size_t a = offset & (align-1);
	return a == 0 ? 0 : (align - a);
}

template<class T>
inline typename std::enable_if<std::is_integral<T>::value, bool>::type
is_pow(T a)
{
	return a && !(a & (a-1U));
}

template<class S, class Q>
struct euleb_count
{
	constexpr static unsigned Sbits = sizeof(S)*8;
	constexpr static unsigned Qbits = sizeof(Q)*8;
	static_assert(Sbits < Qbits);

	enum : unsigned { value = (Qbits%Sbits) == 0 ? Qbits/Sbits : Qbits/Sbits+1 };
};

size_t euleb(size_t s, u8* o);
size_t euleb(size_t s, u16* o);
size_t euleb(size_t s, u32* o);
bool duleb(size_t& s, int& state, u8 v);
bool duleb(size_t& s, int& state, u16 v);
bool duleb(size_t& s, int& state, u32 v);

std::string dm (char const*);
void hexdump (void const* p, int len);

size_t write_file (path_char const* name, u8 const* ptr, size_t size);

}} // pulmotor::util

#endif
