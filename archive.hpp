#ifndef PULMOTOR_ARCHIVE_HPP
#define PULMOTOR_ARCHIVE_HPP

#include "pulmotor_config.hpp"
#include "pulmotor_types.hpp"

#include <type_traits>
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
	size_t offset_;
	
public:
	input_archive (basic_input_buffer& buf) : buffer_ (buf), offset_(0) {}

	enum { is_reading = 1, is_writing = 0 };
	
	template<int Align, class T>
	void read_basic (T& data)
	{
		std::error_code ec;
		int toRead = sizeof(data);
		if (Align != 1)
		{
			char skipBuf[8];
			size_t correctOffset = util::align<Align> (offset_);
			if (int skip = correctOffset - offset_) {
				buffer_.read ((void*)skipBuf, skip, ec);
				offset_ += skip;
			}
		}
		int wasRead = buffer_.read ((void*)&data, toRead, ec);
		offset_ += wasRead;
	}

	void read_data (void* dest, size_t size)
	{
		std::error_code ec;
		int wasRead = buffer_.read (dest, size, ec);
		offset_ += wasRead;
	}
};

class output_archive : public pulmotor_archive
{
	basic_output_buffer& buffer_;
	size_t written_;
	
public:
	output_archive (basic_output_buffer& buf) : buffer_ (buf), written_ (0) {}
	
	enum { is_reading = 0, is_writing = 1 };

	template<int Align, class T>
	void write_basic (T& data)
	{
		std::error_code ec;
		if (Align != 1) {
			char alignBuf[7] = { 0,0,0,0, 0,0,0 };
			size_t correctOffset = util::align<Align> (written_);
			if (int fill = correctOffset - written_) {
				buffer_.write ((void*)alignBuf, fill, ec);
				written_ += fill;
			}
		}
		int written = buffer_.write (&data, sizeof(data), ec);
		written_ += written;
	}

	void write_data (void* src, size_t size)
	{
		std::error_code ec;
		int written = buffer_.write (src, size, ec);
		written_ += written;
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

	template<int Align, class T>
	void read_basic (T& data) {
		if (Align != 1)
			current_ = util::align<Align>(current_);
			
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
	
	template<int Align, class T>
	void write_basic (T& data) {
		if (Align != 1) {
			size_t aligned = util::align<Align> (cont_.size ());
			if (int fill = aligned - cont_.size ())
				cont_.insert(cont_.end (), fill, 0);
		}
		cont_.insert (cont_.end (), (u8 const*)&data, (u8 const*)&data + sizeof(data));
	}
	
	void write_data (void* src, size_t size) {
		cont_.insert (cont_.end (), (u8 const*)src, (u8 const*)src + size);
	}
};


typedef std::true_type true_t;
typedef std::false_type false_t;


// Forward to in-class function if a global one is not available
struct ______CLASS_DOES_NOT_HAVE_AN_ARCHIVE_FUNCTION_OR_MEMBER____ {};

template<class ArchiveT, class T>
inline void has_archive_check_impl (T& obj, true_t)
{}

template<class ArchiveT, class T>
inline void has_archive_check_impl (T& obj, false_t)
{
	typedef std::integral_constant<bool, has_archive_fun<ArchiveT, T>::value> has_archive_t;
	typename boost::mpl::if_<has_archive_t, char, std::pair<T, ______CLASS_DOES_NOT_HAVE_AN_ARCHIVE_FUNCTION_OR_MEMBER____****************> >::type* test = "";
	(void)test;
}

template<class ArchiveT, class T>
inline void call_archive_member_impl (ArchiveT& ar, T& obj, unsigned long version, false_t)
{
	typedef std::integral_constant<bool, has_archive_fun<ArchiveT, T>::value> has_archive_t;
	typename boost::mpl::if_<has_archive_t, char, std::pair<T, ______CLASS_DOES_NOT_HAVE_AN_ARCHIVE_FUNCTION_OR_MEMBER____****************> >::type* test = "";
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
	typedef std::integral_constant<bool, has_archive_fun<ArchiveT, T>::value> has_archive_t;
	//has_archive_check_impl<ArchiveT> (obj, has_archive_t());
	//access::call_archive (ar, obj, version);
	call_archive_member_impl (ar, obj, version, has_archive_t());
}
	
template<class ArchiveT, class T>
inline void redirect_archive (ArchiveT& ar, T const& obj, unsigned version)
{
	typedef typename std::remove_cv<T>::type clean_t;
	archive (ar, *const_cast<clean_t*>(&obj), version);
}

// T is a class
template<class ArchiveT, class T>
inline void dispatch_archive_impl (ArchiveT& ar, T const& obj, unsigned version, true_t, std::integral_constant<int, 1>)
{
	u8 file_version = version;
	if (track_version<typename std::remove_cv<T>::type>::value)
		ar.template read_basic<1> (file_version);
	redirect_archive (ar, obj, file_version);
}

template<class ArchiveT, class T>
inline void dispatch_archive_impl (ArchiveT& ar, T const& obj, unsigned version, false_t, std::integral_constant<int, 1>)
{
	u8 file_version = version;
	if (track_version<typename std::remove_cv<T>::type>::value)
		ar.template write_basic<1> (file_version);
	redirect_archive (ar, obj, version);
}

// T is a primitive
template<class ArchiveT, class T>
inline void dispatch_archive_impl (ArchiveT& ar, T const& obj, unsigned version, true_t, std::integral_constant<int, 0>)
{
	const int Align = std::is_floating_point<T>::value ? sizeof(T) : 1;
	ar.template read_basic<Align> (const_cast<T&> (obj));
}

template<class ArchiveT, class T>
inline void dispatch_archive_impl (ArchiveT& ar, T const& obj, unsigned version, false_t, std::integral_constant<int, 0>)
{
	const int Align = std::is_floating_point<T>::value ? sizeof(T) : 1;
	ar.template write_basic<Align> (obj);
}
	
// T is a wrapped pointer
template<class ArchiveT, class T>
inline void dispatch_archive_impl (ArchiveT& ar, memblock_t<T> const& w, unsigned version, true_t, std::integral_constant<int, 2>)
{
	if (w.addr != 0 && w.count != 0)
		ar.read_data ((void*)w.addr, w.count * sizeof(T));
}

template<class ArchiveT, class T>
inline void dispatch_archive_impl (ArchiveT& ar, memblock_t<T> const& w, unsigned version, false_t, std::integral_constant<int, 2>)
{
	if (w.addr != 0 && w.count != 0)
		ar.write_data ((void*)w.addr, w.count * sizeof(T));
}
	
// T is an array
template<class ArchiveT, class T, int N, class AnyT>
inline void dispatch_archive_impl (ArchiveT& ar, T const (&a)[N], unsigned version, AnyT, std::integral_constant<int, 3>)
{
	for (size_t i=0; i<N; ++i)
		archive (ar, a[i]);
}

// T is a pointer
template<class ArchiveT, class T, class AnyT>
inline void dispatch_archive_impl (ArchiveT& ar, T const& v, unsigned version, AnyT, std::integral_constant<int, 4>)
{
	char************************** ______SERIALIZING_POINTER_IS_NOT_SUPPORTED______[std::is_pointer<T>::value ? -1 : 0];
}
	
// T is enum
//template<class ArchiveT, class T>
//inline void dispatch_archive_impl (ArchiveT& ar, T const& v, unsigned version, AnyT, std::integral_constant<int, 5>)
//{
//	if (w.addr != 0 && w.count != 0)
//		ar.read_data ((void*)w.addr, w.count * sizeof(T));
//}
	

template<class ArchiveT, class T>
void archive (ArchiveT& ar, T const& obj)
{
	typedef typename std::remove_cv<T>::type clean_t;

	typedef
		typename boost::mpl::if_< std::is_array<clean_t>, std::integral_constant<int, 3>,
			typename boost::mpl::if_< std::is_pointer<clean_t>, std::integral_constant<int, 4>,
				typename boost::mpl::if_< std::is_enum<clean_t>, std::integral_constant<int, 0>,
		   			typename boost::mpl::if_< std::is_fundamental<clean_t>, std::integral_constant<int, 0>,
						typename boost::mpl::if_< is_memblock_t<clean_t>, std::integral_constant<int, 2>,
						std::integral_constant<int, 1> >::type
					>::type
				>::type
			>::type
		>::type object_type_t;
	
	typedef std::integral_constant<bool, ArchiveT::is_reading> is_reading_t;
	
	unsigned code_version = version<clean_t>::value;
	dispatch_archive_impl (ar, obj, code_version, is_reading_t(), object_type_t() );
}

template<class ArchiveT, class T>
inline typename pulmotor::enable_if<std::is_base_of<pulmotor_archive, ArchiveT>::value, ArchiveT>::type&
operator& (ArchiveT& ar, T const& obj)
{
	archive (ar, obj);
   	return ar;
}
	
template<class ArchiveT, class T>
inline typename pulmotor::enable_if<std::is_base_of<pulmotor_archive, ArchiveT>::value, ArchiveT>::type&
operator| (ArchiveT& ar, T const& obj)
{
	archive (ar, obj);
	return ar;
}


#define PULMOTOR_ARCHIVE() template<class ArchiveT> void archive (ArchiveT& ar, unsigned version)
#define PULMOTOR_ARCHIVE_SPLIT() template<class ArchiveT> void archive (ArchiveT& ar, unsigned version) {\
	typedef std::integral_constant<bool, ArchiveT::is_reading> is_reading_t; \
	archive_impl(ar, version, is_reading_t()); }
#define PULMOTOR_ARCHIVE_READ() template<class ArchiveT> void archive_impl (ArchiveT& ar, unsigned version, pulmotor::true_t)
#define PULMOTOR_ARCHIVE_WRITE() template<class ArchiveT> void archive_impl (ArchiveT& ar, unsigned version, pulmotor::false_t)
	

#define PULMOTOR_ARCHIVE_FREE(T) template<class ArchiveT> void archive (ArchiveT& ar, T& v, unsigned version)
#define PULMOTOR_ARCHIVE_FREE_SPLIT(T) template<class ArchiveT> inline void archive (ArchiveT& ar, T& v, unsigned version) {\
	typedef std::integral_constant<bool, ArchiveT::is_reading> is_reading_t; \
	archive_impl(ar, v, version, is_reading_t()); }

#define PULMOTOR_ARCHIVE_FREE_READ(T) template<class ArchiveT> inline void archive_impl (ArchiveT& ar, T& v, unsigned version, pulmotor::true_t)
#define PULMOTOR_ARCHIVE_FREE_WRITE(T) template<class ArchiveT> inline void archive_impl (ArchiveT& ar, T& v, unsigned version, pulmotor::false_t)

}

#endif
