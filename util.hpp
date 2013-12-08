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


namespace util {
	
template<class T>
inline T align (T a, size_t alignment)
{
	assert (alignment > 0 && (alignment & (alignment-1)) == 0);
	return (a-1 | alignment-1) + 1;
}

template<int Alignment, class T>
inline T align (T a)
{
	assert (Alignment > 0 && (Alignment & (Alignment-1)) == 0);
	return (a-1 | Alignment-1) + 1;
}
	
template<class T>
inline void blit_to_container (T& a, std::vector<unsigned char>& odata, target_traits const& tt);

std::string dm (char const*);
void hexdump (void const* p, int len);

template<int Size>
struct swap_element_endian;

template <>
struct swap_element_endian<2>
{
	static void swap (void* p) {
		u16 a = *(u16*)p;
		u32 b = ((a & 0x00ff) << 8) | ((a & 0xff00) >> 8);
		*(u16*)p = b;
	}
};

template <>
struct swap_element_endian<4>
{
	static void swap (void* p) {
		u32 a = *(u32*)p;
		u32 b = ((a & 0x000000ff) << 24) | ((a & 0x0000ff00) << 8) | ((a & 0x00ff0000) >> 8) | ((a & 0xff000000) >> 24);
		*(u32*)p = b;
	}
};

template <>
struct swap_element_endian<8>
{
	static void swap (void* p) {
		u64 a = *(u64*)p;
		u64 b = ((a & 0x00000000000000ffLL) << 56)	|
		   		((a & 0x000000000000ff00LL) << 40)	|
			   	((a & 0x0000000000ff0000LL) << 24)	|
			   	((a & 0x00000000ff000000LL) << 8)	|
			   	((a & 0x000000ff00000000LL) >> 8)	|
			   	((a & 0x0000ff0000000000LL) >> 24)	|
			   	((a & 0x00ff000000000000LL) >> 40)	|
			   	((a & 0xff00000000000000LL) >> 56)
				;
		*(u64*)p = b;
	}
};

template<int Size>
inline void swap_endian (void* arg, size_t count)
{
	for (char* p = (char*)arg, *end = (char*)arg + count * Size; p != end; p += Size)
	{
		swap_element_endian<Size>::swap (p);
	}
}

template<class T>
inline void swap_variable (T& a)
{
	swap_element_endian<sizeof(T)>::swap (&a);
}

inline void swap_elements (void* ptr, size_t size, size_t count)
{
	assert (size < 16);
	switch (size)
	{
		default:
			assert ("unsupported primitive type");
			break;

		case 2:
			swap_endian<2> (ptr, count);
			break;
		case 4:
			swap_endian<4> (ptr, count);
			break;
		case 8:
			swap_endian<8> (ptr, count);
			break;
	}
}

void set_offsets (void* data, std::pair<uintptr_t,uintptr_t> const* fixups, size_t fixup_count, size_t ptrsize, bool change_endianess);
void fixup_pointers (void* data, uintptr_t const* fixups, size_t fixup_count);
	
size_t read_file (pp_char const* name, u8 const* ptr, size_t size);
void read_file (pp_char const* name, std::vector<u8>& out, std::error_code& ec);	

size_t write_file (pp_char const* name, u8 const* ptr, size_t size);
	
}}

#endif
