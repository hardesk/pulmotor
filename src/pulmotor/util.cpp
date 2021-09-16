#include "util.hpp" // pulmotor::util
#include <fstream>
#include <string>
#include <unistd.h>
#include <cstdio>

#include "stream.hpp"

#if defined(__GNUC__) && 0
#include <demangle.h>
#endif

#include <cxxabi.h>

namespace pulmotor {
namespace util {

int get_pagesize()
{
	static int ps = 0;
	if (!ps) {
#ifdef _WIN32
		SYSTEM_INFO si;
		GetSystemInfo(&si);
		ps = si.dwPageSize;
#elif defined(__APPLE__)
		ps = getpagesize();
#endif
	}
	return ps;
}

#ifdef __LITTLE_ENDIAN__
#define PULMOTOR_ENDIANESS 0
#else
#define PULMOTOR_ENDIANESS 1
#endif

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

// encode: if don't fit into container (eg. 31bit), set hibit and store the rest as value (eg. 31bit)
template<class T>
inline size_t euleb_impl(size_t s, T* o) {

	static_assert(std::is_unsigned<T>::value, "T must be unsigned");

	using nl = std::numeric_limits<T>;

	unsigned i = 0;
	constexpr unsigned vbits = nl::digits - 1;
	constexpr T hmask = 1U << vbits;
	do {
		o[i] = T(s);
		if ((s >>= vbits))
			o[i] |= hmask;
		++i;
	} while(s);

	return i;
}

// decode: if hibit set, set it to 0 and expect another word
template<class T>
inline bool duleb_impl(size_t& s, int& state, T v) {

	static_assert(std::is_unsigned<T>::value, "T must be unsigned");

	using nl = std::numeric_limits<T>;
	constexpr unsigned vbits = nl::digits - 1;
	constexpr T hmask = 1U << vbits;

	size_t ss = v & ~hmask;
	s |= ss << (state++ * vbits);
	return (v & hmask) != 0;
}

size_t euleb(size_t s, u8* o) { return euleb_impl(s, o); }
size_t euleb(size_t s, u16* o) { return euleb_impl(s, o); }
size_t euleb(size_t s, u32* o) { return euleb_impl(s, o); }
bool duleb(size_t& s, int& state, u8 v) { return duleb_impl(s, state, v); }
bool duleb(size_t& s, int& state, u16 v) { return duleb_impl(s, state, v); }
bool duleb(size_t& s, int& state, u32 v) { return duleb_impl(s, state, v); }

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

size_t write_file (path_char const* name, u8 const* ptr, size_t size)
{
	std::fstream os(name, std::ios_base::binary|std::ios_base::out);
	sink_ostream s(os);
	std::error_code ec;
	s.write(ptr, size, ec);
	return size;
}

} // util

void throw_error(err e, char const* msg, char const* filename, text_location loc)
{
#if PULMOTOR_EXCEPTIONS
	throw yaml_error(e, msg, loc, filename);
#else
	va_list vl;
	va_start(vl, loc);
	vprintf("%s:%d:%d: error(%d): %s\n, filename, loc.line, loc.col, (int)e, msg);
	va_end(vl);
#endif
}

void throw_error(char const* msg, ...)
{
	va_list vl;
	va_start(vl, msg);
#if PULMOTOR_EXCEPTIONS
	char buf[512];
	std::vsnprintf(buf, sizeof buf, msg, vl);
	va_end(vl);
	throw error(err_unspecified, buf);
#else
	vprintf(msg, vl);
	va_end(vl);
#endif
}


void location_map::analyze()
{
	m_lookup.clear();
	for (size_t line = 1, pos = 0; ; ++line) {
		char const* n = ctT::find(m_start + pos, m_size - pos, '\n');
		if (n == nullptr) {
			// in case the string doesn't end on a new line simulate an eol
			if (pos != m_size)
				m_lookup.push_back( info { m_size, line } );
			break;
		}
		size_t np = n - m_start;
		m_lookup.push_back( info { np, line } );
		pos = np + 1;
	}

	assert(check_consistency());
}

bool location_map::check_consistency()
{
	size_t prevL = 0, prevP = 0;
	for (auto const& ii : m_lookup)
	{
		if (ii.line != prevL + 1) return false;
		prevL = ii.line;

		if ( !(prevP <= ii.eol) ) return false;
		prevP = ii.eol;
	}

	return true;
}

text_location
location_map::lookup(size_t offset) {

	if (offset > m_size) {
		assert(false && "line offset out of bounds");
		return text_location{};
	}

	auto it = std::lower_bound(m_lookup.begin(), m_lookup.end(),
		offset,
		[](info const& i, size_t off ) { return i.eol < off; }
	);

	size_t line = 1, search_start = 0, eol;
	if (it != m_lookup.end()) {
		eol = it->eol;
		if (it != m_lookup.begin()) {
			auto p = it;
			--p;
			search_start = p->eol + 1;
			line = p->line + 1;
		}

		if (line == it->line)
			return text_location { unsigned(line), unsigned(offset - search_start + 1) };
	} else {
		size_t len = m_size - offset;
		char const* pnl = (char const*)memchr(m_start + offset, '\n', len);

		eol = pnl == nullptr ? m_size : (pnl - m_start);

		// continue scanning for EOLs starting where we left off.
		if (!m_lookup.empty()) {
			search_start = m_lookup.back().eol + 1;
			line = m_lookup.back().line + 1;
		}
	}

	std::vector<info> ins;
	do {
		size_t len = eol - search_start;
		char const* pnl = (char const*)memchr(m_start + search_start, '\n', len);
		if (!pnl) {
			// we looked till the end of the buffer and found no new line. To
			// avoid repeated scanning append an 'info' with simulated eol at the end of the buffer.
			if (eol == m_size && len != 0) {
				assert( (m_lookup.empty() || m_lookup.back().eol != m_size) && "lookup table already contains end-buffer eol" );
				ins.push_back( info { m_size, line } );
			}

			m_lookup.insert(it, ins.begin(), ins.end());
			assert(check_consistency());
			return text_location { unsigned(line), unsigned(offset - search_start + 1) };
		}

		size_t nl = pnl - m_start;
		ins.push_back( info { nl, line++ } );
		search_start = nl + 1;
	} while (true);
}


} // pulmotor
