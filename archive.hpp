#ifndef PULMOTOR_ARCHIVE_HPP
#define PULMOTOR_ARCHIVE_HPP

#include "pulmotor_config.hpp"
#include "pulmotor_types.hpp"

#include <tr1/type_traits>
#include <boost/mpl/if.hpp>

#include <cstdarg>
#include <cstddef>
#include <cassert>
#include <vector>
#include <algorithm>
#include <list>
#include <set>
#include <map>
#include <typeinfo>
#include <string>

#include "stream_fwd.hpp"
#include "stream.hpp"
#include "util.hpp"

namespace pulmotor {

template<class T>
struct is_memblock_t { enum { value = false }; };

template<class T>
struct is_memblock_t<memblock_t<T> > { enum { value = true }; };

	
template<class ArchiveT, class T>
struct has_archive_fun
{
	template<class U, class Ar, void (U::*)(Ar&, unsigned)> struct tester {};

	template<class U> static char test(tester<U, ArchiveT, &U::archive> const*);
	template<class U> static long test(...);

	static const int value = sizeof(test<T>(0)) == sizeof(char);
};

/*template<class T>
struct version
{
	enum { value = 0 };
};*/
#define PULMOTOR_ARCHIVE_VER(T, v) template<> struct version<T> { enum { value = v }; }

/*template<class T>
struct use_version
{
	enum { value = 1 };
};*/
#define PULMOTOR_ARCHIVE_NOVER(T) template<> struct track_version<T> { enum { value = 0 }; }
#define PULMOTOR_ARCHIVE_NOVER_TEMPLATE1(T) template<class TT> struct track_version< T < TT > > { enum { value = 0 }; }
#define PULMOTOR_ARCHIVE_NOVER_TEMPLATE2(T) template<class TT1, class TT2> struct track_version< T < TT1, TT2 > > { enum { value = 0 }; }
	
	
struct pulmotor_archive { };

class input_archive : public pulmotor_archive
{
	basic_input_buffer& buffer_;
	
public:
	input_archive (basic_input_buffer& buf) : buffer_ (buf) {}

	enum { is_reading = 1, is_writing = 0 };
	
	template<class T>
	void read_basic (T& data)
	{
		error_id err = buffer_.read ((void*)&data, sizeof(data), NULL);
		(void)err;
	}

	void read_data (void* dest, size_t size)
	{
		error_id err = buffer_.read (dest, size, NULL);
		(void)err;
	}
};

class output_archive : public pulmotor_archive
{
	basic_output_buffer& buffer_;
	
public:
	output_archive (basic_output_buffer& buf) : buffer_ (buf) {}
	
	enum { is_reading = 0, is_writing = 1 };

	template<class T>
	void write_basic (T& data)
	{
		error_id err = buffer_.write (&data, sizeof(data), NULL);
		(void)err;
	}

	void write_data (void* src, size_t size)
	{
		error_id err = buffer_.write (src, size, NULL);
		(void)err;
	}
};
	
typedef std::vector<u8> memory_archive_container_t;
class memory_read_archive : public pulmotor_archive
{
	u8 const* data_;
	size_t size_, current_;
public:
	enum { is_reading = 1, is_writing = 0 };
	
	memory_read_archive (u8 const* ptr, size_t s) : data_ (ptr), size_(s), current_(0) {}

	template<class T>
	void read_basic (T& data) {
		if (size_ - current_ >= sizeof(data)) {
			data = *(T const*)(data_ + current_);
			current_ += sizeof(data);
		}
	}
	
	void read_data (void* dst, size_t size) {
		if (size_ - current_ >= size) {
			memcpy (dst, data_ + current_, size);
			current_ += size;
		}
	}
};
	
class memory_write_archive : public pulmotor_archive
{
	memory_archive_container_t& cont_;	
public:
	enum { is_reading = 0, is_writing = 1 };
	
	memory_write_archive (memory_archive_container_t& c) : cont_ (c) {}
	
	template<class T>
	void write_basic (T& data) {
		cont_.insert (cont_.end (), (u8 const*)&data, (u8 const*)&data + sizeof(data));
	}
	
	void write_data (void* src, size_t size) {
		cont_.insert (cont_.end (), (u8 const*)src, (u8 const*)src + size);
	}
};


typedef std::tr1::true_type true_t;
typedef std::tr1::false_type false_t;


// Forward to in-class function if a global one is not available
struct ______CLASS_DOES_NOT_HAVE_AN_ARCHIVE_FUNCTION_OR_MEMBER____ {};

template<class ArchiveT, class T>
inline void has_archive_check_impl (T& obj, true_t)
{}

template<class ArchiveT, class T>
inline void has_archive_check_impl (T& obj, false_t)
{
	typedef std::tr1::integral_constant<bool, has_archive_fun<ArchiveT, T>::value> has_archive_t;
	typename boost::mpl::if_<has_archive_t, char, std::pair<______CLASS_DOES_NOT_HAVE_AN_ARCHIVE_FUNCTION_OR_MEMBER____**********************************, T> >::type* test = "";
	(void)test;
}

template<class ArchiveT, class T>
inline void call_archive_member_impl (ArchiveT& ar, T& obj, unsigned long version, false_t)
{
	typedef std::tr1::integral_constant<bool, has_archive_fun<ArchiveT, T>::value> has_archive_t;
	typename boost::mpl::if_<has_archive_t, char, std::pair<______CLASS_DOES_NOT_HAVE_AN_ARCHIVE_FUNCTION_OR_MEMBER____**********************************, T> >::type* test = "";
	(void)test;
}

template<class ArchiveT, class T>
inline void call_archive_member_impl (ArchiveT& ar, T& obj, unsigned long version, true_t)
{
	access::call_archive (ar, obj, version);
}

template<class ArchiveT, class T>
inline void archive (ArchiveT& ar, T& obj, unsigned long version)
{
	typedef std::tr1::integral_constant<bool, has_archive_fun<ArchiveT, T>::value> has_archive_t;
	//has_archive_check_impl<ArchiveT> (obj, has_archive_t());
	//access::call_archive (ar, obj, version);
	call_archive_member_impl (ar, obj, version, has_archive_t());
}
	
template<class ArchiveT, class T>
inline void redirect_archive (ArchiveT& ar, T const& obj, unsigned version)
{
	typedef typename std::tr1::remove_cv<T>::type clean_t;
	archive (ar, *const_cast<clean_t*>(&obj), version);
}

// T is a class
template<class ArchiveT, class T>
inline void dispatch_archive_impl (ArchiveT& ar, T const& obj, unsigned version, true_t, std::tr1::integral_constant<int, 1>)
{
	u8 file_version = version;
	if (track_version<typename std::tr1::remove_cv<T>::type>::value)
		ar.read_basic (file_version);
	redirect_archive (ar, obj, file_version);
}

template<class ArchiveT, class T>
inline void dispatch_archive_impl (ArchiveT& ar, T const& obj, unsigned version, false_t, std::tr1::integral_constant<int, 1>)
{
	u8 file_version = version;
	if (track_version<typename std::tr1::remove_cv<T>::type>::value)
		ar.write_basic (file_version);
	redirect_archive (ar, obj, version);
}

// T is a primitive
template<class ArchiveT, class T>
inline void dispatch_archive_impl (ArchiveT& ar, T const& obj, unsigned version, true_t, std::tr1::integral_constant<int, 0>)
{
	ar.read_basic (const_cast<T&> (obj));
}

template<class ArchiveT, class T>
inline void dispatch_archive_impl (ArchiveT& ar, T const& obj, unsigned version, false_t, std::tr1::integral_constant<int, 0>)
{
	ar.write_basic (obj);
}
	
// T is a wrapped pointer
template<class ArchiveT, class T>
inline void dispatch_archive_impl (ArchiveT& ar, memblock_t<T> const& w, unsigned version, true_t, std::tr1::integral_constant<int, 2>)
{
	if (w.addr != 0 && w.count != 0)
		ar.read_data ((void*)w.addr, w.count * sizeof(T));
}

template<class ArchiveT, class T>
inline void dispatch_archive_impl (ArchiveT& ar, memblock_t<T> const& w, unsigned version, false_t, std::tr1::integral_constant<int, 2>)
{
	if (w.addr != 0 && w.count != 0)
		ar.write_data ((void*)w.addr, w.count * sizeof(T));
}
	
// T is an array
template<class ArchiveT, class T, int N, class AnyT>
inline void dispatch_archive_impl (ArchiveT& ar, T const (&a)[N], unsigned version, AnyT, std::tr1::integral_constant<int, 3>)
{
	for (size_t i=0; i<N; ++i)
		archive (ar, a[i]);
}

// T is a pointer
template<class ArchiveT, class T, class AnyT>
inline void dispatch_archive_impl (ArchiveT& ar, T const& v, unsigned version, AnyT, std::tr1::integral_constant<int, 4>)
{
	char************************** ______SERIALIZING_POINTER_IS_NOT_SUPPORTED______[std::tr1::is_pointer<T>::value ? -1 : 0];
}
	
// T is enum
//template<class ArchiveT, class T>
//inline void dispatch_archive_impl (ArchiveT& ar, T const& v, unsigned version, AnyT, std::tr1::integral_constant<int, 5>)
//{
//	if (w.addr != 0 && w.count != 0)
//		ar.read_data ((void*)w.addr, w.count * sizeof(T));
//}
	

template<class ArchiveT, class T>
void archive (ArchiveT& ar, T const& obj)
{
	typedef typename std::tr1::remove_cv<T>::type clean_t;

	typedef
		typename boost::mpl::if_< std::tr1::is_array<clean_t>, std::tr1::integral_constant<int, 3>,
			typename boost::mpl::if_< std::tr1::is_pointer<clean_t>, std::tr1::integral_constant<int, 4>,
				typename boost::mpl::if_< std::tr1::is_enum<clean_t>, std::tr1::integral_constant<int, 0>,
		   			typename boost::mpl::if_< std::tr1::is_fundamental<clean_t>, std::tr1::integral_constant<int, 0>,
						typename boost::mpl::if_< is_memblock_t<clean_t>, std::tr1::integral_constant<int, 2>,
						std::tr1::integral_constant<int, 1> >::type
					>::type
				>::type
			>::type
		>::type object_type_t;
	
	typedef std::tr1::integral_constant<bool, ArchiveT::is_reading> is_reading_t;
	
	unsigned code_version = version<clean_t>::value;
	dispatch_archive_impl (ar, obj, code_version, is_reading_t(), object_type_t() );
}

template<class ArchiveT, class T>
inline typename pulmotor::enable_if<std::tr1::is_base_of<pulmotor_archive, ArchiveT>::value, ArchiveT>::type&
operator& (ArchiveT& ar, T const& obj)
{
	archive (ar, obj);
   	return ar;
}
	
template<class ArchiveT, class T>
inline typename pulmotor::enable_if<std::tr1::is_base_of<pulmotor_archive, ArchiveT>::value, ArchiveT>::type&
operator| (ArchiveT& ar, T const& obj)
{
	archive (ar, obj);
	return ar;
}


#define PULMOTOR_ARCHIVE() template<class ArchiveT> void archive (ArchiveT& ar, unsigned version)
#define PULMOTOR_ARCHIVE_SPLIT(T) template<class ArchiveT> inline void archive (ArchiveT& ar, unsigned version) {\
	typedef std::tr1::integral_constant<bool, ArchiveT::is_reading> is_reading_t; \
	archive_impl(ar, version, is_reading_t()); }
#define PULMOTOR_ARCHIVE_READ() template<class ArchiveT> inline void archive_impl (ArchiveT& ar, unsigned version, pulmotor::true_t)
#define PULMOTOR_ARCHIVE_WRITE() template<class ArchiveT> inline void archive_impl (ArchiveT& ar, unsigned version, pulmotor::false_t)
	

#define PULMOTOR_ARCHIVE_FREE(T) template<class ArchiveT> void archive (ArchiveT& ar, T& v, unsigned version)
#define PULMOTOR_ARCHIVE_FREE_SPLIT(T) template<class ArchiveT> inline void archive (ArchiveT& ar, T& v, unsigned version) {\
	typedef std::tr1::integral_constant<bool, ArchiveT::is_reading> is_reading_t; \
	archive_impl(ar, v, version, is_reading_t()); }

#define PULMOTOR_ARCHIVE_FREE_READ(T) template<class ArchiveT> inline void archive_impl (ArchiveT& ar, T& v, unsigned version, pulmotor::true_t)
#define PULMOTOR_ARCHIVE_FREE_WRITE(T) template<class ArchiveT> inline void archive_impl (ArchiveT& ar, T& v, unsigned version, pulmotor::false_t)

}

#endif
