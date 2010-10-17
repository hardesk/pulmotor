#include "util.hpp"
#include <string>

#if defined(__GNUC__) && 0
#include <demangle.h>
#endif

namespace pulmotor {
	
#ifdef __LITTLE_ENDIAN__
#define PULMOTOR_ENDIANESS 0
#else
#define PULMOTOR_ENDIANESS 1
#endif	

// TODO: this should go to another file
target_traits const target_traits::current = { sizeof(void*), PULMOTOR_ENDIANESS };

target_traits const target_traits::le_lp32 = { 4, false };
target_traits const target_traits::be_lp32 = { 4, true };

target_traits const target_traits::le_lp64 = { 8, false };
target_traits const target_traits::be_lp64 = { 8, true };
	
namespace util {

std::string dm (char const* a)
{
#if defined(__GNUC__) && 0
	return cplus_demangle (a, 0);
#else
	return a;
#endif
}


inline char fixchar (unsigned char c)
{
	if (c < 32 || c > 128)
		return '.';
	return c;
}


void hexdump (void const* p, int len)
{
	char const* src = (char const*)p;
	int const ll = 8;
	for (int i=0; i<len;)
	{
		if (i % ll == 0)
			printf ("0x%06x: ", (unsigned)i);

		int cnt = std::min (len-i, ll);

		for (int j=0; j<cnt; ++j)
			printf ("%02x ", (unsigned char)src[i+j]);
		
		printf ("   ");
		for (int j=0; j<cnt; ++j)
			printf ("%c", fixchar ((unsigned char)src[i+j]));

		i += cnt;

		printf ("\n");
	}
}

void set_offsets (void* data, std::pair<uintptr_t,uintptr_t> const* fixups, size_t fixup_count, int ptrsize, bool change_endianess)
{
	char* datap = (char*)data;
	for (size_t i=0; i<fixup_count; ++i)
	{
		switch (ptrsize)
		{
			default:
			case 2:
//				assert ("unsupported pointer size");
				break;

			case 4:
				{
					u32* p = reinterpret_cast<u32*> (datap + fixups[i].first);
					*p = (uintptr_t)fixups[i].second;
					if (change_endianess)
						swap_endian<sizeof(*p)> (p, 1);
				}
				break;
			case 8:
				{
					u64* p = reinterpret_cast<u64*> (datap + fixups[i].first);
					*p = (uintptr_t)fixups[i].second;
					if (change_endianess)
						swap_endian<sizeof(*p)> (p, 1);
				}
				break;
		}
	}
}

void fixup_pointers (void* data, uintptr_t const* fixups, size_t fixup_count)
{
	char* datap = (char*)data;
	for (size_t i=0; i<fixup_count; ++i)
	{
		uintptr_t* p = reinterpret_cast<uintptr_t*> (datap + fixups[i]);
		*p += (uintptr_t)data;
	}
}

void* fixup_pointers_impl (blit_section_info* bsi)
{
	char* data = ((char*)bsi + bsi->data_offset);
	uintptr_t* fixups = (uintptr_t*)((char*)bsi + bsi->fixup_offset);
	
	util::fixup_pointers (data, fixups, bsi->fixup_count);
	
	return data;
}

}}
