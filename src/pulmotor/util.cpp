#include "util.hpp"
#include <string>
#include "stream.hpp"

#if defined(__GNUC__) && 0
#include <demangle.h>
#endif

#include <cxxabi.h>

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
		
#define PULMOTOR_BN1(x) char const* short_type_name<x>::name = #x;
	
	PULMOTOR_BN1(char)
	PULMOTOR_BN1(unsigned char)
	PULMOTOR_BN1(short)
	PULMOTOR_BN1(unsigned short)
	PULMOTOR_BN1(int)
	PULMOTOR_BN1(unsigned)
	PULMOTOR_BN1(long)
	PULMOTOR_BN1(unsigned long)
	PULMOTOR_BN1(long long)
	PULMOTOR_BN1(unsigned long long)
	PULMOTOR_BN1(float)
	PULMOTOR_BN1(double)
	PULMOTOR_BN1(long double)
	PULMOTOR_BN1(char16_t)
	PULMOTOR_BN1(char32_t)
	
	
std::string abi_demangle(char const* mangled)
{
	size_t len = 0;
	int status = 0;
	char* demangled = abi::__cxa_demangle (mangled, NULL, &len, &status);
	std::string ret (status == 0 ? demangled : mangled);
	free (demangled);
	return ret;
}
	
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

void set_offsets (void* data, std::pair<uintptr_t,uintptr_t> const* fixups, size_t fixup_count, size_t ptrsize, bool change_endianess)
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
					*p = (u32)fixups[i].second;
					if (change_endianess)
						swap_endian<sizeof(*p)> (p, 1);
				}
				break;
			case 8:
				{
					u64* p = reinterpret_cast<u64*> (datap + fixups[i].first);
					*p = (u64)fixups[i].second;
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
	
size_t read_file (pp_char const* name, u8* ptr, size_t size, std::error_code& ec)
{
	std::auto_ptr<basic_input_buffer> input = create_plain_input (name);
	
	if (input.get ())
		return input->read (ptr, size, ec);
	else
		ec = std::make_error_code(std::errc::io_error);
	
	return 0;
}
	
void read_file (pp_char const* name, std::vector<u8>& out, std::error_code& ec)
{
	if (size_t fl = (size_t)get_file_size (name))
	{
		out.resize (fl);
		size_t wasRead = read_file (name, out.data(), fl, ec);
		out.resize (wasRead);
	}
	else
		ec = std::make_error_code(std::errc::no_such_file_or_directory);
}
	
size_t write_file (pp_char const* name, u8 const* ptr, size_t size)
{
	std::auto_ptr<basic_output_buffer> output = create_plain_output (name);
	
	if (output.get ())
	{
		std::error_code ec;
		size_t written = output->write (ptr, size, ec);
		if (ec)
			return 0;
		return written;
	}
	
	return 0;
}

	

}}
