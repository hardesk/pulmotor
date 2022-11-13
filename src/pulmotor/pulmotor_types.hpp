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
enum : u16
{
	version_dont_track			= 0xffffu, // depracated
	no_version					= 0xffffu, // value that is transformed into ver_flag_no_version
	version_default				= 0u,

	// object pointer was null when writing
	ver_flag_null_ptr			= 0x8000u,

	// vu follows 'version' that specifies how much to advance stream (this u32 not included) to get to object data
	// in other words how much unknown information is between the version and object
	ver_flag_garbage_length		= 0x4000u,

	// arbitraty "debug" string is present after (optional garbage length)
	ver_flag_debug_string		= 0x2000u,
	ver_debug_string_max_size	= 0x100u,

	// align object (except primitive types) on an alignment address specified by archive
	ver_flag_align_object		= 0x1000u,

	// set if object was serialized using "wants-construct == true"
	ver_flag_wants_construct	= 0x0200u,

	// class is configured not to store the version into file
	ver_flag_no_version			= 0x0100u,

	// mask to get only the version part
	ver_mask					= 0x00ffu,
	ver_mask_bits				= 8
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

template<class T, class = void>
struct get_meta : std::integral_constant<unsigned,
	(unsigned)class_version<T>::value == (unsigned)no_version
		? (unsigned)ver_flag_no_version|0u
		: (unsigned)class_version<T>::value>
{};
template<class T>
struct get_meta<T, std::void_t< decltype(T::version) > > : std::integral_constant<unsigned,
	(unsigned)T::version == (unsigned)no_version
		? (unsigned)ver_flag_no_version|0u
		: (unsigned)T::version>
{};

#define PULMOTOR_VERSION(T, v) template<> struct ::pulmotor::class_version<T> { enum { value = v }; }

struct romu3;
struct romu_q32;

unsigned rand_range(romu3& rnd, unsigned range);
unsigned rand_range(romu_q32& rnd, unsigned range);

struct romu3
{
	using result_type = uint64_t;
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

	unsigned range(unsigned mx) { return rand_range(*this, mx); }
};

struct romu_q32
{
	u32 state[4];

	using result_type = u32;

	constexpr static result_type min() { return 0; }
	constexpr static result_type max() { return UINT32_MAX; }

	constexpr void seed(u32 w, u32 x, u32 y, u32 z) {
		state[0] = w;
		state[1] = x;
		state[2] = y;
		state[3] = z;
	}

	constexpr void init() {
		seed(0xdbf64230, 0x9c248ed8, 0x6a11e8b8, 0x9bca720f);
	}

	constexpr u32 operator()() {
		uint32_t wp = state[0], xp = state[1], yp = state[2], zp = state[3];
		state[0] = 3323815723u * zp;  // a-mult
		state[1] = zp + PULMOTOR_ROTL(wp,26);  // b-rotl, c-add
		state[2] = yp - xp;           // d-sub
		state[3] = yp + wp;           // e-add
		state[3] = PULMOTOR_ROTL(state[3],9);    // f-rotl
		return xp;
	}

	unsigned range(unsigned mx) { return rand_range(*this, mx); }
};

struct object_meta
{
	object_meta() : flags_and_version(0) {}
	object_meta(unsigned flags_and_ver) : flags_and_version(flags_and_ver) {}
	object_meta(object_meta const&) = default;
	object_meta& operator=(object_meta const&) = default;

	unsigned version() const { return flags_and_version & ver_mask; }
	unsigned is_nullptr() const { return flags_and_version & ver_flag_null_ptr; }
	bool include_version() const { return (flags_and_version & ver_flag_no_version) == 0; }
	void set_version(unsigned ver) { flags_and_version = (flags_and_version&~ver_mask) | (ver&ver_mask); }

	operator unsigned() const { return flags_and_version & ver_mask; }

private:
	unsigned flags_and_version; // version and flags
};

template<class T>
struct nv_t
{
	using value_type = T;

	char const* name;
	T& obj;
	size_t name_length;

	nv_t(char const* name_, size_t namelength, T const& o)
		: name(name_), name_length(namelength), obj(const_cast<T&>(o))
		{}
	nv_t(nv_t const& o) = default;
};

template<class T>
inline nv_t<T> nvl( char const* name, T const& a) { return nv_t<T>(name, strlen(name), a); }

template<class T, size_t N>
inline nv_t<T> nv( char const (&name)[N], T const& a) { return nv_t<T>(name, N, a); }

template<class T>
struct is_nv : std::false_type {};

template<class T>
struct is_nv<nv_t<T>> : std::true_type {};

template<class T, class D>	struct delete_holder		 { void de(T* p) const { d(p); } D const& d; };
template<class T> 			struct delete_holder<T,void> { void de(T*) const { } };

template<class T, class C, class D = delete_holder<T, void> >
struct ctor : D
{
	static_assert(std::is_same<T, typename std::remove_cv<T>::type>::value);

	using type = T;

	T** ptr;
	C const& cf;

	ctor(T** p, C const& c) : ptr(p), cf(c) {}
	ctor(T** p, C const& c, D const& d) : D(d), ptr(p), cf(c) {}

	template<class... Args> auto operator()(Args&&... args) const { return cf(args...); }
};

template<class T, class C, class D = delete_holder<T, void> >
struct ctor_pure : D
{
	static_assert(std::is_same<T, typename std::remove_cv<T>::type>::value);

	using type = T;
	C const& cf;

	ctor_pure(C const& c) : cf(c) {}
	ctor_pure(C const& c, D const& d) : D(d), cf(c) {}

	template<class... Args> auto operator()(Args&&... args) const { return cf(args...); }
};



#define PULMOTOR_implPSTR(x) #x
#define PULMOTOR_PSTR(x) PULMOTOR_implPSTR(x)
#define PULMOTOR_implPCAT(m, x) m ## x
#define PULMOTOR_PCAT(m, x) PULMOTOR_implPCAT(m, x)
#define PULMOTOR_implPSIZE(a, b, c, d, e, f, g, h, size, ...) size
#define PULMOTOR_PSIZE(...) PULMOTOR_implPSIZE(__VA_ARGS__, 8, 7, 6, 5, 4, 3, 2, 1, 0)
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
