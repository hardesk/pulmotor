#ifndef PULMOTOR_TYPES_HPP_
#define PULMOTOR_TYPES_HPP_

#include <string>
#include <type_traits>

namespace pulmotor
{
typedef unsigned char		u8;
typedef signed char			s8;
typedef unsigned short int	u16;
typedef signed short int	s16;
typedef unsigned int		u32;
typedef signed int			s32;
typedef unsigned long long	u64;
typedef signed long long	s64;

typedef std::true_type true_t;
typedef std::false_type false_t;

#ifdef _WIN32
typedef wchar_t path_char;
typedef std::wstring path_string;
#else
typedef char path_char;
typedef std::string path_string;
#endif

typedef u64 fs_t;

enum { header_size = 8 };
enum : unsigned
{
	version_dont_track			= 0xffff'ffffu,
	no_version					= 0xffff'ffffu, // value that is transformed into ver_flag_no_version
	version_default				= 0u,

	// object pointer was null when writing
	ver_flag_null_ptr			= 0x8000'0000u,

	// u32 follows 'version' that specifies how much to advance stream (this u32 not included) to get to object data
	// in other words how much unknown information is between the version and object
	ver_flag_garbage_length		= 0x4000'0000u,

	// arbitraty "debug" string is present after (optional garbage length)
	ver_flag_debug_string		= 0x2000'0000u,
	ver_debug_string_max_size	= 0x100u,

	// align object on an alignment address specified by archive
	ver_flag_align_object		= 0x1000'0000u,

	// set if object was serialized using "wants-construct == true"
	ver_flag_wants_construct	= 0x0100'0000u,

	// class is configured not to store the version into file
	ver_flag_no_version			= 0x0200'0000u,

	// mask to get only the version part
	ver_mask					= 0x00ffffffu
};


struct target_traits
{
	// size of the pointer in bits
	size_t	ptr_size;
	bool	big_endian;

	static target_traits const current;

	static target_traits const le_lp32;
	static target_traits const be_lp32;

	static target_traits const le_lp64;
	static target_traits const be_lp64;
};

template<class T, class = void> struct has_version_member : std::false_type {};
template<class T>               struct has_version_member<T, std::void_t<char [(std::is_convertible<decltype(T::version), unsigned>::value) ? 1 : -1]> > : std::true_type {};

template<class T> struct class_version { static unsigned constexpr value = version_default; };

template<class T, class = void> struct get_meta : std::integral_constant<unsigned,
	(unsigned)class_version<T>::value == (unsigned)no_version ? (unsigned)ver_flag_no_version|0u : (unsigned)class_version<T>::value> {};
template<class T>               struct get_meta<T, std::void_t< decltype(T::version) > > : std::integral_constant<unsigned,
	(unsigned)T::version == (unsigned)no_version ? (unsigned)ver_flag_no_version|0u : (unsigned)T::version> {};

#define PULMOTOR_VERSION(T, v) template<> struct ::pulmotor::class_version<T> { enum { value = v }; }

struct romu3
{
	typedef uint64_t result_type;
	constexpr static result_type min() { return 0; }
	constexpr static result_type max() { return UINT64_MAX; }

	#define PULMOTOR_ROTL(d,lrot) ((d<<(lrot)) | (d>>(8*sizeof(d)-(lrot))))
	uint64_t xState=0xe2246698a74e50e0ULL, yState=0x178cd4541df4e31cULL, zState=0x704c7122f9cfbd76ULL;
	void seed(uint64_t a, uint64_t b,uint64_t c) { xState=a; yState=b; zState=c; }
	void reset() { *this = romu3(); }
	uint64_t operator() () {
		uint64_t xp = xState, yp = yState, zp = zState;
		xState = 15241094284759029579u * zp;
		yState = yp - xp;  yState = PULMOTOR_ROTL(yState,12);
		zState = zp - yp;  zState = PULMOTOR_ROTL(zState,44);
		return xp;
	}
	unsigned r(unsigned range) { return uint64_t(unsigned(operator()())) * range >> (sizeof (unsigned)*8); }
};

template<class T>
struct nv_t
{
	char const* name;
	T const& obj;

	nv_t(char const* name_, T const& o) : name(name_), obj(o) {}
	nv_t(nv_t const& o) : name(o.name), obj(o.obj) {}
};

template<class T>
inline nv_t<T> nv(char const* name, T const& o)
{
	return nv_t<T> (name, o);
}
#define PULMOTOR_iPSTR(x) #x
#define PULMOTOR_PSTR(x) PULMOTOR_iPSTR(x)
#define PULMOTOR_iPCAT(m, x) m ## x
#define PULMOTOR_PCAT(m, x) PULMOTOR_iPCAT(m, x)
#define PULMOTOR_iPSIZE(a, b, c, d, e, f, g, h, size, ...) size
#define PULMOTOR_PSIZE(...) PULMOTOR_iPSIZE(__VA_ARGS__, 8, 7, 6, 5, 4, 3, 2, 1, 0)
#define PULMOTOR_POVERLOAD(m, ...) PULMOTOR_PCAT(m, PULMOTOR_PSIZE(__VA_ARGS__))(__VA_ARGS__)

#define PULMOTOR_NV_IMPL_1(property) ::pulmotor::nv(#property, property)
#define PULMOTOR_NV_IMPL_2(name, property) ::pulmotor::nv(name, property)
#define NV(...) PULMOTOR_OVERLOAD(PULMOTOR_NV_IMPL_, __VA_ARGS__)

template<class T> struct is_nvp : public std::false_type {};
template<class T> struct is_nvp<nv_t<T>> : public std::true_type {};


/*
template<class AsT, class ActualT>
struct as_holder
{
	ActualT& m_actual;
	explicit as_holder (ActualT& act) : m_actual(act) {}
};

template<class AsT, class ActualT>
inline as_holder<AsT, ActualT> as (ActualT& act)
{
	// Actual may be const qualified. We actually want that so that as works when writing, with const types
	return as_holder<AsT, ActualT> (act);
}*/

} // pulmotor

#endif // PULMOTOR_TYPES_HPP_
