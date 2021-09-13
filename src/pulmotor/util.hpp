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


template<class T>
inline typename std::enable_if<std::is_unsigned<T>::value, T>::type
align(T offset, size_t al)
{
	T mask = al-1U;
	return ((offset-1U) | mask) + 1U;
}

template<int Alignment, class T>
inline typename std::enable_if<std::is_unsigned<T>::value, T>::type
align (T off)
{
	assert (Alignment > 0 && (Alignment & (Alignment-1)) == 0);
	return (off-1 | Alignment-1) + 1;
}

template<class T>
inline typename std::enable_if<std::is_integral<T>::value, bool>::type
is_aligned(T offset, size_t align)
{
	return (offset & (align-1U)) == 0;
}

inline size_t pad(size_t offset, size_t align)
{
	size_t a = offset & (align-1);
	return a == 0 ? 0 : (align - a);
}

template<class T>
inline typename std::enable_if<std::is_integral<T>::value, bool>::type
is_pow(T a)
{
	return a && !(a & (a-1U));
}

template<class T>
struct fun_traits;

template<size_t I, class T, class... Args> struct arg_ii { using type = typename arg_ii<I-1, Args...>::type; };
template<class T, class... Args> struct arg_ii<0U, T, Args...> { using type = T; };
template<size_t I, class... Args> struct arg_i { using type = typename arg_ii<I, Args...>::type; };
template<size_t I> struct arg_i<I> { using type = void; };

template<class R, class T, class... Args>
struct fun_traits<R (T::*)(Args...)>
{
    static constexpr unsigned arity = sizeof...(Args);
    using return_type = R;
    using class_type = T;
    template<size_t I> using arg = typename arg_i<I, Args...>::type;
};

template<class R, class... Args>
struct fun_traits<R (*)(Args...)>
{
    static constexpr unsigned arity = sizeof...(Args);
    using return_type = R;
    template<size_t I> using arg = typename arg_i<I, Args...>::type;
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

struct text_location
{
	unsigned line, column;
	bool operator==(text_location const&) const = default;
};

struct location_map
{
	// mac: \n (10)
	// win: \r\n (13) (10)
	char const* m_start;
	size_t m_size;

	struct info { size_t eol, line; };
	std::vector<info> m_lookup;

	using ctT = std::char_traits<char>;

	location_map(char const* start, size_t len) : m_start(start), m_size(len) {}

	void analyze();
	bool check_consistency();

	text_location lookup(size_t offset);
};

template<class S, class Q>
struct euleb_count
{
	constexpr static unsigned Sbits = sizeof(S)*8;
	constexpr static unsigned Qbits = sizeof(Q)*8;
	static_assert(Sbits < Qbits);

	enum : unsigned { value = (Qbits%Sbits) == 0 ? Qbits/Sbits : Qbits/Sbits+1 };
};

size_t euleb(size_t s, u8* o);
size_t euleb(size_t s, u16* o);
size_t euleb(size_t s, u32* o);
bool duleb(size_t& s, int& state, u8 v);
bool duleb(size_t& s, int& state, u16 v);
bool duleb(size_t& s, int& state, u32 v);

std::string dm (char const*);
void hexdump (void const* p, int len);

size_t write_file (path_char const* name, u8 const* ptr, size_t size);

}} // pulmotor::util

#endif
