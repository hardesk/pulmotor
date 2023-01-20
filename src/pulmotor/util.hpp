#ifndef PULMOTOR_UTILS_HPP_
#define PULMOTOR_UTILS_HPP_

#include "pulmotor_config.hpp"
#include "pulmotor_types.hpp"
#include <cstring>
#include <string_view>
#include <vector>
#include <algorithm>
#include <utility>
#include <stdint.h>
#include <cstdio>
#include <cassert>
#include <system_error>

namespace pulmotor {

namespace util {

PULMOTOR_ATTR_DLL int get_pagesize();
PULMOTOR_ATTR_DLL std::string abi_demangle(char const* name);

template<class T>
inline std::string readable_name ()
{
    typedef typename std::remove_cv<T>::type clean_t;
    return abi_demangle (typeid(clean_t).name());
}

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


template<std::integral T>
constexpr inline T //typename std::enable_if<std::is_unsigned<T>::value, T>::type
align(T offset, size_t al)
{
    T mask = al-1U;
    return ((offset-1U) | mask) + 1U;
}

template<size_t Alignment, std::integral T>
constexpr inline T //typename std::enable_if<std::is_unsigned<T>::value, T>::type
align (T offset)
{
    assert (Alignment > 0 && (Alignment & (Alignment-1)) == 0);
    return (offset-1 | Alignment-1) + 1;
}

template<std::integral T>
constexpr inline bool //typename std::enable_if<std::is_integral<T>::value, bool>::type
is_aligned(T offset, size_t align)
{
    return (offset & (align-1U)) == 0;
}

constexpr inline size_t pad(size_t offset, size_t align)
{
    size_t a = offset & (align-1);
    return a == 0 ? 0 : (align - a);
}

template<class T>
constexpr inline typename std::enable_if<std::is_integral<T>::value, bool>::type
is_pow(T a)
{
    return a && !(a & (a-1U));
}

template<class T>
struct fun_traits;

namespace impl {
    template<size_t I, class T, class... Args>	struct at_ii { using type = typename at_ii<I-1, Args...>::type; };
    template<class T, class... Args>			struct at_ii<0U, T, Args...> { using type = T; };
}

template<size_t I, class... Args> struct at_i { using type = typename impl::at_ii<I, Args...>::type; };
template<size_t I> 				  struct at_i<I> { using type = void; };

template<class R, class T, class... Args>
struct fun_traits<R (T::*)(Args...)>
{
    static constexpr unsigned arity = sizeof...(Args);
    using return_type = R;
    using class_type = T;
    template<size_t I> using arg = typename at_i<I, Args...>::type;
};

template<class R, class... Args>
struct fun_traits<R (*)(Args...)>
{
    static constexpr unsigned arity = sizeof...(Args);
    using return_type = R;
    template<size_t I> using arg = typename at_i<I, Args...>::type;
};


// adapted from https://codereview.stackexchange.com/a/193436
template<class F, class Tuple, size_t... Is>
inline auto map_impl(F&& f, Tuple&& tup, std::index_sequence<Is...>)
{
    if constexpr (sizeof...(Is) == 0)
        return std::tuple<>{};
    // return void when function returns void
    else if constexpr (std::is_same< decltype(f(std::get<0>(tup))), void >::value) {
        (f(std::get<Is>(tup)), ...);
        return;
    }
    // when returning a reference, return a tuple of references instead of making copies
    else if constexpr (std::is_lvalue_reference_v< decltype(f(std::get<0>(tup))) >)
        return std::tie (f(std::get<Is>(tup))... );
    // or forward when the function returns &&
    else if constexpr (std::is_rvalue_reference_v< decltype(f(std::get<0>(tup))) >)
        return std::forward_as_tuple (f(std::get<Is>(tup))... );
    // otherwise keep the results by value and return a tuple
    else
        return std::tuple( f(std::get<Is>(tup))... );
}

// basically `map' calls f on each of the tuple elements and returns a tuple with return values
template<class F, class... Args>
auto map(F&& f, std::tuple<Args...> const& tup)
{
    return map_impl( std::forward<F>(f), tup, std::make_index_sequence<sizeof...(Args)>());
}

template<class F, class... Args>
auto map(F&& f, std::tuple<Args...>& tup)
{
    return map_impl( std::forward<F>(f), tup, std::make_index_sequence<sizeof...(Args)>());
}

template<class Store, class Quantity>
struct euleb_count
{
    constexpr static unsigned Sbits = sizeof(Store)*8;
    constexpr static unsigned Qbits = sizeof(Quantity)*8;
    static_assert(Sbits <= Qbits);

    enum : unsigned { value = (Qbits%Sbits) == 0 ? Qbits/Sbits : Qbits/Sbits+1 };
};

size_t euleb(size_t s, u8* o);
size_t euleb(size_t s, u16* o);
size_t euleb(size_t s, u32* o);
bool duleb(size_t& s, int& state, u8 v);
bool duleb(size_t& s, int& state, u16 v);
bool duleb(size_t& s, int& state, u32 v);

constexpr static const size_t base64_invalid = (size_t)-1LL;
enum class base64_options : unsigned
{
    nopad = 0x0,
    skip_whitespace = 0x01,
    ignore_illegal = 0x02,
    pad = 0x04,
    // wrap_n = 0x10,
    // wrap_mask = 0xffff0000
};
constexpr inline base64_options operator|(base64_options a, base64_options b) { return base64_options((unsigned)a | (unsigned)b); }
constexpr inline unsigned operator&(base64_options a, base64_options b) { return (unsigned)a & (unsigned)b; }
// constexpr inline base64_options base64_options_wrap(unsigned N) { return (base64_options)((N << 16) | (unsigned)base64_options::wrap_n); }
// constexpr inline unsigned base64_options_get_wrap(base64_options opts) { return (unsigned)opts >> 16; }

size_t base64_encode_length(size_t raw_length, base64_options opts = base64_options::pad);
void base64_encode(char const* src, size_t raw_length, char* dest, base64_options opts = base64_options::pad);
size_t base64_decode_length_approx(size_t encoded_length);
size_t base64_decode(char const* src, size_t encoded_length, char* dest, base64_options opts = base64_options::skip_whitespace);

std::string dm (char const*);
void hexdump (void const* p, int len);
std::string hexstring (void const* p, size_t len);

size_t write_file (path_char const* name, u8 const* ptr, size_t size);

} // util

namespace tl {

namespace impl {

template<class... Ts> struct index_impl;
template<class T> struct index_impl<T> : std::integral_constant<size_t, 0> {};
template<class T, class... Ts> struct index_impl<T, T, Ts...> : std::integral_constant<size_t, 0U> {};
template<class T, class U, class... Ts> struct index_impl<T, U, Ts...> : std::integral_constant<size_t, 1 + index_impl<T, Ts...>::value> {};

} // impl

template<class... Ts> struct list {

    template<class T> using append = list<Ts..., T>;
    template<class T> using prepend = list<T, Ts...>;
    template<class T> using index = std::integral_constant<size_t, impl::index_impl<T, Ts...>::value>;
    template<class T> using has = std::integral_constant<bool, (std::is_same_v<T, Ts> || ...)>;
    using size = std::integral_constant<size_t, sizeof...(Ts)>;

};

template<class T, class L> using has = typename L::template has<T>;
template<class T, class L> using index = typename L::template index<T>;

}

template<class F>
struct scope_exit
{
    scope_exit(F&& f)
        noexcept( std::is_nothrow_copy_constructible<F>::value )
        : fun( std::forward<std::decay_t<F>>(f) )
    {}
    scope_exit(scope_exit<F>&& a)
        noexcept( std::is_nothrow_move_constructible<F>::value )
        : fun(std::move(a.fun)), suspended(a.suspended) {
            a.suspended = true;
    }
    ~scope_exit() noexcept {
        if (!suspended)
            fun();
    }

    scope_exit(scope_exit const&) = delete;
    scope_exit const& operator=(scope_exit const&) = delete;
    scope_exit const& operator=(scope_exit const&&) = delete;

    void release() noexcept { suspended = true; }

private:
    std::decay_t<F> fun;
    bool suspended = false;
};

template<class F>
scope_exit(F) -> scope_exit<F>;

#define PULMOTOR_SCOPE_EXIT1(C, ...) \
    pulmotor::scope_exit scope_exit_##C = [__VA_ARGS__]()

#define PULMOTOR_SCOPE_EXIT(...) \
    PULMOTOR_SCOPE_EXIT1(__COUNTER__, __VA_ARGS__)


struct text_location
{
    unsigned line, column;
    bool operator==(text_location const&) const = default;
};

struct full_location : text_location
{
    char const* name;
    full_location() : text_location{0,0}, name(nullptr) {}
    full_location(char const* name, unsigned line, unsigned col) : text_location{line, col}, name(name) {}
    full_location(char const* name, text_location const& l) : text_location{l.line, l.column}, name(name) {}
    bool operator==(full_location const&) const = default;
};

struct location_map
{
    using ctT = std::char_traits<char>;

    location_map(char const* start, size_t len);
    ~location_map();

    void analyze();
    text_location lookup(size_t offset);
    size_t line_count() const { return m_lookup.size(); }

private:
    bool check_consistency();

    // mac: \n (10)
    // win: \r\n (13) (10)
    char const* m_start;
    size_t m_size;

    struct info { size_t eol, line; };
    std::vector<info> m_lookup;
};


template<class T, class D>
struct context {
    D& data;
    T& obj;
};

template<class T, class D>
constexpr inline context<T, D> with_ctx (T& o, D&& data) {
    return context<T, D> { o, std::forward<D> (data) };
}

#if PULMOTOR_EXCEPTIONS

enum err
{
    err_unspecified,
    err_bad_stream,
    err_unexpected_value_type,
    err_key_not_found,
    err_value_out_of_range,
    err_invalid_value
};

struct error : std::runtime_error
{
    error(err c, std::string const& msg) : std::runtime_error(msg), code(c) {}
    err code;
};

struct yaml_error : error
{
    text_location location;
    std::string filename;
    yaml_error(err c, std::string const& msg, text_location const& l, std::string const& filename_)
    :	error(c, msg)
    ,	location(l)
    ,	filename(filename_)
    {}
};

#endif

PULMOTOR_ATTR_DLL void throw_error(err e, char const* msg, char const* filename, text_location loc);
PULMOTOR_ATTR_DLL void throw_error(char const* msg, ...);
PULMOTOR_ATTR_DLL void throw_system_error(std::error_code const& ec, char const* msg, char const* filename, text_location loc);
#define PULMOTOR_THROW_SYSTEM_ERROR(ec, msg) throw_system_error(ec, msg, __FILE__, text_location { __LINE__, 0 } )
PULMOTOR_ATTR_DLL std::string ssprintf(char const* msg, ...);

namespace lit
{
constexpr inline std::ptrdiff_t operator"" _z(unsigned long long a) { return std::ptrdiff_t(a); }
constexpr inline std::size_t operator"" _uz(unsigned long long a) { return std::size_t(a); }
constexpr inline std::size_t operator"" _zu(unsigned long long a) { return std::size_t(a); }
}

} // pulmotor

#endif
