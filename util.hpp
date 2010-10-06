#ifndef PULMOTOR_UTILS_HPP_
#define PULMOTOR_UTILS_HPP_

#include "pulmotor_config.hpp"
#include <cstring>
#include <algorithm>
#include <utility>
#include <stdint.h>
#include <cstdio>
#include <cassert>

namespace pulmotor { namespace util {

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

void set_offsets (void* data, std::pair<uintptr_t,uintptr_t> const* fixups, size_t fixup_count, int ptrsize, bool change_endianess);
void fixup_pointers (void* data, uintptr_t const* fixups, size_t fixup_count);

}}

#endif
