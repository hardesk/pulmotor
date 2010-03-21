#include "util.hpp"
#include <string>

#if defined(__GNUC__) && 0
#include <demangle.h>
#endif

namespace pulmotor { namespace util {

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

void set_offsets (void* data, std::pair<uintptr_t,uintptr_t> const* fixups, size_t fixup_count, bool change_endianess)
{
	char* datap = (char*)data;
	for (size_t i=0; i<fixup_count; ++i)
	{
		uintptr_t* p = reinterpret_cast<uintptr_t*> (datap + fixups[i].first);
		*p = (uintptr_t)fixups[i].second;
		if (change_endianess)
			swap_endian<sizeof(*p)> (p, 1); 
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










}}
