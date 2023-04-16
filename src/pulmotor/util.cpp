#include "util.hpp" // pulmotor::util
#include <fstream>
#include <string>
#include <unistd.h>
#include <cstdio>

#include "stream.hpp"

#define PULMOTOR_DEMANGLE_SUPPORT 1

#if PULMOTOR_DEMANGLE_SUPPORT
#include <cxxabi.h>
// #include <demangle.h>
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

// encode quantity as a series of T (LSB first) where hi bit idicates (==1) if next T value needs to be processed when decoding.
template<class T>
inline size_t euleb_impl(size_t quantity, T* o) {

    static_assert(std::is_unsigned<T>::value, "T must be unsigned");

    using nl = std::numeric_limits<T>;

    unsigned i = 0;
    constexpr unsigned vbits = nl::digits - 1;
    constexpr T hmask = 1U << vbits;
    do {
        o[i] = T(quantity);
        if ((quantity >>= vbits))
            o[i] |= hmask;
        ++i;
    } while(quantity);

    return i;
}

// decode: hi-bit indicates (==1) if more data is expected. On finish, `state' holds decoded length. state must be zero on first call.
template<class T>
inline bool duleb_impl(size_t& quantity, int& state, T v) {

    static_assert(std::is_unsigned<T>::value, "T must be unsigned");

    using nl = std::numeric_limits<T>;
    constexpr unsigned vbits = nl::digits - 1;
    constexpr T hmask = 1U << vbits;

    size_t ss = v & ~hmask;
    quantity |= ss << (state++ * vbits);
    return (v & hmask) != 0;
}

size_t euleb(size_t s, u8* o) { return euleb_impl(s, o); }
size_t euleb(size_t s, u16* o) { return euleb_impl(s, o); }
size_t euleb(size_t s, u32* o) { return euleb_impl(s, o); }
bool duleb(size_t& s, int& state, u8 v) { return duleb_impl(s, state, v); }
bool duleb(size_t& s, int& state, u16 v) { return duleb_impl(s, state, v); }
bool duleb(size_t& s, int& state, u32 v) { return duleb_impl(s, state, v); }

size_t base64_encode_length(size_t raw_length, base64_options opts) {
    size_t trips = raw_length / 3;
    size_t rem = raw_length % 3; // 0: 0, 1: 2, 2: 3
    if (opts & base64_options::pad)
        return (trips + (rem != 0)) * 4;
    else
        return trips * 4 + (rem == 0 ? 0 : (rem + 1));
}

// on x64, table based approach is ~7x faster
#define PULMOTOR_BASE64_METHOD 0

static char const b64etab[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdef"
    "ghijklmnopqrstuvwxyz0123456789+/";
constexpr static char base64enc_byte(unsigned value) { return b64etab[value]; }
// return value < 26 ? 'A' + value
//     : value < 52 ? 'a' - 26 + value
//     : value < 62 ? '0' - 52 + value
//     : value < 63 ? '+' : '/';

void base64_encode(char const* src, size_t raw_length, char* dest, base64_options opts) {
    size_t mult3 = raw_length / 3 * 3, rem = raw_length % 3, i = 0;
    for (; i<mult3; i += 3) {
        u8 a=src[i+0], b=src[i+1], c=src[i+2];
        // *AAAAAA *AABBBB *BBBBCC *CCCCCC
        *dest++ = base64enc_byte(a >> 2);
        *dest++ = base64enc_byte((a&0x3)<<4 | (b&0xf0)>>4);
        *dest++ = base64enc_byte((b&0xf)<<2 | (c&0xc0)>>6);
        *dest++ = base64enc_byte(c&0x3f);
    }

    if (rem) {
        assert (rem <= 2);

        u8 a=src[i+0], b=0;
        *dest++ = base64enc_byte(a >> 2);
        if (rem > 1) b=src[i+1];
        *dest++ = base64enc_byte((a&0x3)<<4 | (b&0xf0)>>4);
        if (rem == 2) *dest++ = base64enc_byte((b&0xf)<<2); else if (opts & base64_options::pad) *dest++ = '=';
        if (opts & base64_options::pad) *dest++ = '=';
    }
}

size_t base64_decode_length_approx(size_t enc_length)
{
    return util::align<4>(enc_length) / 4 * 3;
}

static u8 b64dec_tab[256] =
{
    128, 128, 128, 128, 128, 128, 128, 128, 128,  64,  64,  64,  64,  64, 128, 128,
    128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
    64, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,  62, 128,  62, 128,  63,
    52,  53,  54,  55,  56,  57,  58,  59,  60,  61, 128, 128, 128,  64, 128, 128,
    128,   0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,
    15,  16,  17,  18,  19,  20,  21,  22,  23,  24,  25, 128, 128, 128, 128,  63,
    128,  26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40,
    41,  42,  43,  44,  45,  46,  47,  48,  49,  50,  51, 128, 128, 128, 128, 128,
    128, 128, 128, 128, 128,  64, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
    128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
    64, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
    128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
    128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
    128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
    128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
    128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128
};

//PULMOTOR_NOINLINE
static size_t reload_values(char const* src, size_t& i, size_t len, u8 (&x)[4], base64_options opts) {
    using namespace pulmotor::lit;
    size_t q = 0;
    bool gotAlignment = false;
    for ( ; i < len && q < 4; ++i) {
        u8 c = (u8)src[i];
        if (c == '=') {
            gotAlignment = true;
            continue;
        }

        u8 xx = b64dec_tab[c];
        if ((xx & 0x40) && (opts & base64_options::skip_whitespace)) // skippable character
            continue;
        else if ((xx & 0x80) && !(opts & base64_options::ignore_illegal)) // unexpected/invalid character
            return base64_invalid;
        else if (gotAlignment)
            return q;
        else
            x[q++] = xx;
    }
    return q;
}

size_t base64_decode(char const* src, size_t encoded_length, char* dest, base64_options opts)
{
    using namespace pulmotor::lit;

    size_t i = 0;
    size_t n=0;
    char* o = dest;
    u8 x[4];
    for(; i + 3 < encoded_length; ) {
        u8 s[4] = { (u8)src[i+0], (u8)src[i+1], (u8)src[i+2], (u8)src[i+3] };
        x[0] = b64dec_tab[ s[0] ];
        x[1] = b64dec_tab[ s[1] ];
        x[2] = b64dec_tab[ s[2] ];
        x[3] = b64dec_tab[ s[3] ];

        if ( (x[0]|x[1]|x[2]|x[3]) >= 64) {
            n = reload_values(src, i, encoded_length, x, opts);
            if (n == base64_invalid)
                break;
            if (n < 4 && i >= encoded_length) // if we hit the end of the total data, exit loop
                goto have_data;
        } else
            i += 4;

        *o++ = (x[0] << 2) | (x[1] >> 4);
        *o++ = (x[1] << 4) | (x[2] >> 2);
        *o++ = (x[2] << 6) | x[3];
    }

    n = 0;

have_data:

    if (i < encoded_length) {
        assert(n == 0);
        n = reload_values(src, i, encoded_length, x, opts);
    }
    if (n == base64_invalid) return base64_invalid;

    assert(n < 4 && "tail shall contain less than 4 characters");

    if (n) {
        *o++ = (x[0] << 2) | (x[1] >> 4);
        if (n > 2) {
            *o++ = (x[1] << 4) | (x[2] >> 2);
            if (n > 3)
                *o++ = (x[2] << 6) | x[3];
        }
    }

    return o - dest;
}

std::string dm (char const* a)
{
#if PULMOTOR_DEMANGLE_SUPPORT
    // return cplus_demangle (a, 0);
    char output[1024];
    size_t len = sizeof(output);
    int status = 0;
    char* realname = abi::__cxa_demangle(a, output, &len, &status);
    if (status == 0)
        return std::string(realname, len);
    else
        return std::string(a);
#else
    return a;
#endif
}

inline char fixchar (unsigned char c)
{
    if (c < 32)
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

std::string hexstring (void const* p, size_t len)
{
    std::string ret;
    char const* src = (char const*)p;
    for (size_t i=0; i<len; ++i)
    {
        char buff[4];
        snprintf(buff, sizeof buff, "%02x ", (unsigned)(unsigned char)src[i]);
        ret += std::string_view(buff, i == len-1 ? 2 : 3);
    }
    return ret;
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
    vprintf("%s:%d:%d: error(%d): %s\n", filename, (int)loc.line, (int)loc.col, (int)e, msg);
#endif
}

void throw_fmt_error(char const* filename, text_location loc, char const* msg, ...)
{
    va_list vl;
    va_start(vl, msg);
#if PULMOTOR_EXCEPTIONS
    char buf[512];
    if (int w = std::snprintf(buf, sizeof buf, "%s:%d: ", filename, loc.line)) {
        std::vsnprintf(buf + w, sizeof buf - w, msg, vl);
    }
    va_end(vl);
    throw error(err_unspecified, buf);
#else
    vprintf(msg, vl);
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

void throw_system_error(std::error_code const& ec, char const* msg, char const* filename, text_location loc)
{
#if PULMOTOR_EXCEPTIONS
    char buf[512];
    std::snprintf(buf, sizeof buf, "%s:%d:%d %s", filename, loc.line, loc.column, msg);
    throw std::system_error(ec, buf);
#else
    vprintf(msg, vl);
    va_end(vl);
#endif
}

std::string ssprintf(char const* msg, ...)
{
    va_list vl;
    va_start(vl, msg);
    char buf[512];
    snprintf(buf, sizeof buf, msg, vl);
    va_end(vl);
    return buf;
}

location_map::location_map(char const* start, size_t len)
:	m_start(start), m_size(len)
{
}

location_map::~location_map()
{
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
