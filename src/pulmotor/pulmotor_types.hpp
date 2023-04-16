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

typedef u64 fs_t; // type used to store file size

enum : u16 {
    header_size         = 8u,
    default_version		= 1u,
};

enum class vfl : u16
{

    // object pointer was null when writing
    null_ptr		        	=  0x8000u,

    // u16 follows 'version' that specifies how much to advance stream (u16 value not included) to get to object data.
    // in other words, how much unknown information is between the version and the object data
    garbage                     = 0x4000u,

    // arbitraty "debug" string is present after (optional garbage length)
    debug_string		        = 0x2000u,
    debug_string_max_size	    = 0x100u,

    // align object (except primitive types) on an alignment address specified by archive
    align_object        		= 0x1000u,

    // set if object was serialized using "wants-construct == true"
    wants_construct	            = 0x0200u,

    none                        = 0x0
};
constexpr inline vfl operator|(vfl a, vfl b) { return (vfl)( (unsigned)a | (unsigned)b); }
constexpr inline vfl operator&(vfl a, vfl b) { return (vfl)( (unsigned)a & (unsigned)b); }
constexpr inline vfl operator~(vfl a) { return (vfl)(~(unsigned)a); }
constexpr inline bool operator==(vfl a, unsigned b) { return (unsigned)a == b; }
constexpr inline bool operator!=(vfl a, unsigned b) { return (unsigned)a != b; }
constexpr inline vfl& operator|=(vfl& a, vfl b) { a = (vfl)((unsigned)a | (unsigned)b); return a; }
constexpr inline vfl& operator&=(vfl& a, vfl b) { a = (vfl)((unsigned)a & (unsigned)b); return a; }

constexpr static const u16 no_prefix = (u16)0;

#define PULMOTOR_TEXT_INTERNAL_PREFIX "pulM"

struct prefix
{
    constexpr bool store() const { return version != no_prefix; }

    constexpr bool is_default_version() const { return version == (unsigned)default_version; }

    constexpr bool is_nullptr() const { return (flags & vfl::null_ptr) != 0; }
    constexpr bool wants_construct() const { return (flags & vfl::wants_construct) != 0; }

    constexpr bool has_garbage() const { return (flags & vfl::garbage) != 0; }
    constexpr bool has_dbgstr() const { return (flags & vfl::debug_string) != 0; }
    constexpr bool is_aligned() const { return (flags & vfl::align_object) != 0; }

    constexpr prefix reflag(vfl set, vfl clear = vfl::none) const {
        return prefix { version, (flags & ~clear) | set };
    }

    bool operator==(prefix const&) const = default;

    u16 version;
    vfl flags;
};

struct prefix_ext
{
    std::string class_name;
//    std::string comment;
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

template<class T> struct class_version	: std::integral_constant<unsigned, default_version> {};
template<class T> struct class_flags	: std::integral_constant<vfl, vfl::none> {};
template<class T> struct type_align	    : std::integral_constant<unsigned, alignof(T)> {};

// template<> struct type_format<vec4, yaml_archive> : std::integral_constant<unsigned, yaml::fmt_flags::flow> {};

template<class Ar, class T> struct is_primitive_type : std::integral_constant<bool, std::is_arithmetic<T>::value> {};

// define version as no_prefix to disable writing a prefix
#define PULMOTOR_DECL_VERSION(T, V) \
    template<> struct ::pulmotor::class_version<T> : std::integral_constant<unsigned, (unsigned)V> {}

// #define PULMOTOR_DECL_FLAGS(T, F) \
//     template<> struct ::pulmotor::class_flags<T> : std::integral_constant<vfl, F> {}

// #define PULMOTOR_DECL_VERFLAGS(T, V, F) \
//     template<> struct ::pulmotor::class_version<T> : std::integral_constant<unsigned, (unsigned)V> {}; \
//     template<> struct ::pulmotor::class_flags<T> : std::integral_constant<unsigned, (unsigned)F> {}

namespace impl
{
    template<class T, class = void > struct has_version_member : std::false_type {};
    template<class T>                struct has_version_member<T, std::void_t< decltype(T::version) > > : std::true_type {};

    template<class T>
    consteval u16 get_version() {
        if constexpr (requires { T::serialize_version; std::convertible_to<decltype(T::serialize_version), unsigned>; })
            return T::serialize_version;
        else if constexpr (requires { T::version; std::convertible_to<decltype(T::version), unsigned>; })
            return T::version;
        else
            return class_version<T>::value;
    }
    // template<class T, class = void > struct version : std::integral_constant<unsigned, class_version<T>::value> {};
    // template<class T>                struct version<T, std::void_t<decltype(T::version)> > : std::integral_constant<unsigned, T::version> {};

    template<class T>
    struct flags : std::integral_constant<vfl, vfl::none> {};
}

template<class T>
struct type_util
{
    using has_version_member = impl::has_version_member<T>;
    using version = std::integral_constant<unsigned, impl::get_version<T>()>;
    using flags = impl::flags<T>;
    using align = type_align<T>;

    // customization point to treat a specific type as a primitive for a specific archive
    template<class Ar>
    using is_primitive = is_primitive_type<Ar, T>;
    template<class Ar> static constexpr bool is_primitive_v = is_primitive<Ar>::value;

    static constexpr prefix extract_prefix() { return prefix{ version::value, flags::value }; }
    static constexpr bool store_prefix() { return version::value != (unsigned)no_prefix; }
};

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

template<class T>
struct nv_t
{
    using value_type = T;

    char const* name;
    size_t name_length;
    T obj;

    // nv_t(char const* name_, size_t namelength, T const& o)
    //     : name(name_), name_length(namelength), obj(const_cast<T&>(o))
    //     {}
    // nv_t(nv_t const& o) = default;

    std::string_view get_nameview() const { return std::string_view(name, name_length); }
};

template<class T>
constexpr inline nv_t<T> nvl( char const* name, T const& a) { return nv_t<T>(name, strlen(name), a); }

template<class T, size_t N>
    requires (std::is_scalar<T>::value)
// constexpr inline nv_t<T> nv( char const (&name)[N], T const& a) { return nv_t<T>(name, N - 1, a); }
constexpr inline nv_t<T> nv( char const (&name)[N], T a) { return nv_t<T>{ name, N-1, a }; }

template<class T, size_t N>
    requires (!std::is_scalar<T>::value)
// constexpr inline nv_t<T> nv( char const (&name)[N], T const& a) { return nv_t<T>(name, N - 1, a); }
constexpr inline nv_t<T&> nv( char const (&name)[N], T const& a) { return nv_t<T&>{ name, N-1, const_cast<T&>(a) }; }

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
