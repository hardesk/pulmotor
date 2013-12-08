#ifndef PULMOTOR_ARCHIVE_HPP
#define PULMOTOR_ARCHIVE_HPP

#include "pulmotor_config.hpp"
#include "pulmotor_fwd.hpp"
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
	
template<class ArchiveT, class T>
void archive (ArchiveT& ar, T const& obj);
	
typedef u8 version_t;

template<class T>
struct is_memblock_t { enum { value = false }; };
template<class T>
struct is_memblock_t<memblock_t<T> > { enum { value = true }; };

	
template<class T>
struct nv_impl
{
	char const* name;
	T& obj;
	
	nv_impl(char const* name_, T& o) : name(name_), obj(o) {}
};

#define nv1(x) ::pulmotor::nv_impl<decltype(x)>(#x, x)
#define nv(x) nv1(x)
#define nv_(n,x) ::pulmotor::nv_impl<decltype(x)>(n, x)

template<class T> struct is_nvp : public std::false_type {};
template<class T> struct is_nvp<nv_impl<T>> : public std::true_type {};
	
template<class ArchiveT, class T>
struct has_archive_fun
{
	template<class U, class Ar, void (U::*)(Ar&, unsigned)> struct tester {};

	template<class U> static char test(tester<U, ArchiveT, &U::archive> const*);
	template<class U> static long test(...);

	static const int value = sizeof(test<T>(0)) == sizeof(char);
};

template<class ArchiveT, class T>
struct has_serialize_fun
{
	template<class U, class Ar, void (U::*)(Ar&, unsigned)> struct tester {};
	
	template<class U> static char test(tester<U, ArchiveT, &U::serialize> const*);
	template<class U> static long test(...);
	
	static const int value = sizeof(test<T>(0)) == sizeof(char);
};
	
template<class ArchiveT, class T>
struct has_static_archive_construct
{
	template<class U, class Ar, void (Ar&, T*, unsigned)> struct tester {};
	
	template<class U> static char test(tester<U, ArchiveT, &U::archive_construct> const*);
	template<class U> static long test(...);
	
	static const int value = sizeof(test<T>(0)) == sizeof(char);
};

template<class ArchiveT, class T>
struct has_archive_save_construct
{
	template<class U, class Ar, void (U::*)(Ar&, unsigned)> struct tester {};
	
	template<class U> static char test(tester<U, ArchiveT, &U::archive_save_construct> const*);
	template<class U> static long test(...);
	
	static const int value = sizeof(test<T>(0)) == sizeof(char);
};
		
struct pulmotor_archive
{
	pulmotor_archive() {}
	~pulmotor_archive() {}
	
	pulmotor_archive(pulmotor_archive const&) = delete;
	pulmotor_archive& operator=(pulmotor_archive const&) = delete;
	
	void begin_array () {}
	void end_array () {}
	void begin_object () {}
	void end_object () {}
	
	void object_name(char const*) {}
};
	
template<class T, class... DataT>
object_context<T, DataT...> with_ctx (T& o, DataT... data)
{
	return object_context<T, DataT...> (o, data...);
}
	
template<class BaseT, class TupleT>
class archive_with_context : public pulmotor_archive, object_context_tag
{
	typedef TupleT context_t;
	
	BaseT& m_base;
	context_t const& m_ctx;
	
public:
	enum { is_reading = BaseT::is_reading, is_writing = BaseT::is_writing };
	typedef BaseT base_t;
	
	size_t offset() const { return m_base.offset(); }
	
	context_t const& ctx () const { return m_ctx; }
	
	archive_with_context (base_t& ar, context_t const& ctx) : m_base(ar), m_ctx(ctx) {}
	
	void begin_array () { m_base.begin_array(); }
	void end_array () { m_base.end_array(); }
	void begin_object () { m_base.begin_object(); }
	void end_object () { m_base.end_object(); }
	
	void object_name(char const* name) { m_base.object_name (name); }
	
	
	void do_version (version_t& ver) { m_base.do_version(ver); }
	
	template<int Align, class T>
	void write_basic (T a) { m_base.template write_basic<Align>(a); }
	void write_data (void* dest, size_t size) { m_base.write_data(dest, size); }
	
	template<int Align, class T>
	void read_basic (T& data) { m_base.template read_basic<Align>(data); }
	void read_data (void* dest, size_t size) { m_base.read_data(dest, size); }
};

	// for now we simply derive the archive from object_context_tag to mark it as carrying a contetx
template<class Arch>
	struct is_context_carrier : public std::is_base_of<object_context_tag, Arch>::type {};

class input_archive : public pulmotor_archive
{
	basic_input_buffer& buffer_;
	size_t offset_;
	
public:
	input_archive (basic_input_buffer& buf) : buffer_ (buf), offset_(0) {}
	
	size_t offset() const { return offset_; }

	enum { is_reading = 1, is_writing = 0 };
	std::error_code ec;
	
	void do_version (version_t& ver) {
		read_basic<1> (ver);
	}
	
	template<int Align, class T>
	void read_basic (T& data)
	{
		if (ec)
			return;
		
		int toRead = sizeof(data);
		if (Align != 1)
		{
			char skipBuf[8];
			size_t correctOffset = util::align<Align> (offset_);
			if (size_t skip = correctOffset - offset_) {
				buffer_.read ((void*)skipBuf, skip, ec);
				offset_ += skip;
			}
		}
		size_t wasRead = buffer_.read ((void*)&data, toRead, ec);
		offset_ += wasRead;
	}

	void read_data (void* dest, size_t size)
	{
		if (ec)
			return;
		size_t wasRead = buffer_.read (dest, size, ec);
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
	
	size_t offset() const { return written_; }
	
	std::error_code ec;
	
	void do_version (version_t& ver) {
		write_basic<1> (ver);
	}

	template<int Align, class T>
	void write_basic (T& data)
	{
		if (ec)
			return;
		
		if (Align != 1) {
			char alignBuf[7] = { 0,0,0,0, 0,0,0 };
			size_t correctOffset = util::align<Align> (written_);
			if (size_t fill = correctOffset - written_) {
				buffer_.write ((void*)alignBuf, fill, ec);
				if (ec)
					return;
				written_ += fill;
			}
		}
		size_t written = buffer_.write (&data, sizeof(data), ec);
		written_ += written;
	}

	void write_data (void* src, size_t size)
	{
		if (ec)
			return;
		
		size_t written = buffer_.write (src, size, ec);
		written_ += written;
	}
};

template<class R>
class debug_archive : public pulmotor_archive
{
	R& m_actual_arch;
	std::ostream& m_out;
	int m_indent = 0;
	char const* m_name = nullptr;
	
public:
	
	debug_archive (R& actual_arch, std::ostream& out) : m_actual_arch(actual_arch), m_out(out) {}
	
	enum { is_reading = R::is_reading, is_writing = R::is_writing };
	
	void begin_array () { m_indent++; m_name = NULL; }
	void end_array () { m_indent--;  m_name = NULL; }
	void begin_object () { m_indent++; }
	void end_object () { m_indent--; m_name = NULL; }
	
	size_t offset () const { return m_actual_arch.offset(); }
	
	void object_name(char const* name) { m_name = name; }
	
	void do_version (version_t& ver) {
		size_t off = offset();
		m_actual_arch.do_version (ver);
		print_name ("__version", ver, off);
	}
	
	template<class T>
	void print_name(char const* name, T const& data, size_t off)
	{
		char buf[128];
		snprintf (buf, sizeof(buf), "%*s0x%08x %s (%s)", m_indent*2, "",
				  (int)off,
				  name ? name : "",
				  short_type_name<typename std::remove_cv<T>::type>::name
		);
		m_out << buf << std::to_string(data) << std::endl;
	}
	
	template<int Align, class T>
	void write_basic (T& data)
	{
		print_name (m_name, data, offset());
		m_actual_arch.template write_basic<Align> (data);
	}
	
	void write_data (void* src, size_t size)
	{
		char buf[128];
		snprintf (buf, sizeof(buf), "%*s0x%08x data, %d bytes (%7.1fkB)\n", m_indent*2, "",
				  (int)offset(), (int)size, size / 1024.0f);
		m_out << buf;
		
		m_actual_arch.write_data (src, size);
	}
	
	template<int Align, class T>
	void read_basic (T& data)
	{
		size_t off = offset();
		m_actual_arch.template read_basic<Align> (data);
		print_name (m_name, data, off);
	}
	
	void read_data (void* src, size_t size)
	{
		char buf[128];
		snprintf (buf, sizeof(buf), "%*s0x%08x data, %d bytes (%7.1fkB)\n", m_indent*2, "",
				  offset(), (int)size, size / 1024.0f);
		m_out << buf;

		m_actual_arch.read_data (src, size);
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
	
	void do_version (version_t& ver) {
		read_basic<1> (ver);
	}

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
	
	void do_version (version_t& ver) {
		write_basic<1> (ver);
	}
	
	template<int Align, class T>
	void write_basic (T& data) {
		if (Align != 1) {
			size_t aligned = util::align<Align> (cont_.size ());
			if (size_t fill = aligned - cont_.size ())
				cont_.insert(cont_.end (), fill, 0);
		}
		cont_.insert (cont_.end (), (u8 const*)&data, (u8 const*)&data + sizeof(data));
	}
	
	void write_data (void* src, size_t size) {
		cont_.insert (cont_.end (), (u8 const*)src, (u8 const*)src + size);
	}
};

// Forward to in-class function if a global one is not available
struct CLASS_DOES_NOT_HAVE_AN_ARCHIVE_FUNCTION_OR_MEMBER {};

template<class ArchiveT, class T>
inline void has_archive_check_impl (T& obj, true_t)
{}

template<class ArchiveT, class T>
inline void has_archive_check_impl (T& obj, false_t)
{
	typedef std::integral_constant<bool, has_archive_fun<ArchiveT, T>::value> has_archive_t;
	typename boost::mpl::if_<has_archive_t, char, std::pair<T, CLASS_DOES_NOT_HAVE_AN_ARCHIVE_FUNCTION_OR_MEMBER****************> >::type* test = "";
	(void)test;
}

template<class ArchiveT, class T>
inline void call_archive_member_impl (ArchiveT& ar, T& obj, unsigned long version, std::integral_constant<int, 0>)
{
	typedef std::integral_constant<bool, has_archive_fun<ArchiveT, T>::value> has_archive_t;
	typename boost::mpl::if_<has_archive_t, char, std::pair<T, CLASS_DOES_NOT_HAVE_AN_ARCHIVE_FUNCTION_OR_MEMBER****************> >::type* test = "";
	(void)test;
}

template<class ArchiveT, class T>
inline void call_archive_member_impl (ArchiveT& ar, T& obj, unsigned long version, std::integral_constant<int, 1>)
{
	access::call_archive (ar, obj, (unsigned)version);
}

template<class ArchiveT, class T>
inline void call_archive_member_impl (ArchiveT& ar, T& obj, unsigned long version, std::integral_constant<int, 2>)
{
	access::call_serialize(ar, obj, (unsigned)version);
}

template<class ArchiveT, class T>
inline void archive (ArchiveT& ar, T& obj, unsigned long version)
{
	typedef std::integral_constant<bool, has_archive_fun<ArchiveT, T>::value> has_archive_t;
	typedef std::integral_constant<bool, has_serialize_fun<ArchiveT, T>::value> has_serialize_t;
	
	typedef std::integral_constant<int, has_archive_t::value ? 1 : has_serialize_t::value ? 2 : 0> has_arch_or_ser_t;
	//typedef std::integral_constant<int, 1> has_arch_or_ser_t;
	
	//has_archive_check_impl<ArchiveT> (obj, has_archive_t());
	//access::call_archive (ar, obj, version);
	//	call_archive_member_impl (ar, obj, version, has_arch_or_ser_t());
	call_archive_member_impl (ar, obj, version, has_arch_or_ser_t());
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
	version_t file_version = version;
	if (track_version<typename std::remove_cv<T>::type>::value)
		ar.do_version (file_version);
	ar.begin_object();
	redirect_archive (ar, obj, file_version);
	ar.end_object();
}

template<class ArchiveT, class T>
inline void dispatch_archive_impl (ArchiveT& ar, T const& obj, unsigned version, false_t, std::integral_constant<int, 1>)
{
	version_t file_version = version;
	if (track_version<typename std::remove_cv<T>::type>::value)
		ar.do_version (file_version);
	ar.begin_object();
	redirect_archive (ar, obj, version);
	ar.end_object();
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
	inline void memblock_archive_impl (ArchiveT& ar, T* p, size_t count, unsigned version, true_t, true_t /*is_fundamental*/) {
		ar.read_data ((void*)p, count * sizeof(T));
	}
	
	template<class ArchiveT, class T>
	inline void memblock_archive_impl (ArchiveT& ar, T* p, size_t count, unsigned version, false_t, true_t /*is_fundamental*/) {
		ar.write_data ((void*)p, count * sizeof(T));
	}
	
	template<class ArchiveT, class T>
	inline void memblock_archive_impl (ArchiveT& ar, T* p, size_t count, unsigned version, true_t, false_t) {
		version_t file_version = version;
		if (track_version<typename std::remove_cv<T>::type>::value)
			ar.do_version (file_version);
		
		ar.begin_array();
		for (size_t i=0; i<count; ++i)
			redirect_archive(ar, p[i], version);
		ar.end_array();
	}
	
	template<class ArchiveT, class T>
	inline void memblock_archive_impl (ArchiveT& ar, T* p, size_t count, unsigned version, false_t /*is_reading*/, false_t /*is_fundamental*/) {
		version_t file_version = version;
		if (track_version<typename std::remove_cv<T>::type>::value)
			ar.do_version (file_version);
		
		ar.begin_array();
		for (size_t i=0; i<count; ++i)
			redirect_archive(ar, p[i], version);
		ar.end_array();
	}
	
	
template<class ArchiveT, class T, class AnyT>
inline void dispatch_archive_impl (ArchiveT& ar, memblock_t<T> const& w, unsigned version, AnyT, std::integral_constant<int, 2>)
{
	if (w.addr != 0 && w.count != 0)
	{
		typedef std::integral_constant<bool, ArchiveT::is_reading> is_reading_t;
		typedef std::integral_constant<bool, std::is_fundamental<T>::value> is_fund_t;
		
		memblock_archive_impl (ar, (T*)w.addr, w.count, version, is_reading_t(), is_fund_t());
	}
}

/*template<class ArchiveT, class T>
inline void dispatch_archive_impl (ArchiveT& ar, memblock_t<T> const& w, unsigned version, false_t, std::integral_constant<int, 2>)
{
	if (w.addr != 0 && w.count != 0)
		ar.write_data ((void*)w.addr, w.count * sizeof(T));
}*/
	
// T is an array
template<class ArchiveT, class T, int N, class AnyT>
inline void dispatch_archive_impl (ArchiveT& ar, T const (&a)[N], unsigned version, AnyT, std::integral_constant<int, 3>)
{
	ar.begin_array();
	for (size_t i=0; i<N; ++i)
		archive (ar, a[i]);
	ar.end_array();
}

// Default version for constructing an object
template<class ArchiveT, class T>
inline void archive_construct (ArchiveT& ar, T* p, unsigned long version)
{
	// by default we create an object and serialize its members
	new (p)T();
	archive (ar, *p, version);
}

template<class ArchiveT, class T>
inline void invoke_archive_construct(ArchiveT& ar, T* o, unsigned version, std::true_type /* has member construct*/)
{
	T::archive_construct (ar, o, version);
}

#if 1
	template<class ArchiveT, class T>
inline void invoke_archive_construct(ArchiveT& ar, T* o, unsigned version, std::false_type /* non member construct*/)
{
	archive_construct (ar, o, version);
}
#endif
	
template<class ArchiveT, class T>
inline void select_archive_construct(ArchiveT& ar, T* o, unsigned version)
{
	// here we call one of invoke_archive_construct versions. In case where member function is not available
	// the call ends up either in default archive_construct, or in user-defined archive_construct according
	// to 'version' type matching
	typedef std::integral_constant<bool, has_static_archive_construct<ArchiveT, T>::value> has_memfun_t;
	
	//	char CLASS_DOEST_HAVE_SAVE_CONTRUCT_FUNCTION[has_memfun_t::value ? 1 : -1];
	
	invoke_archive_construct (ar, o, version, has_memfun_t());
}
	

//template<class ArchiveT, class T>
//inline void dispatch_archive_impl (ArchiveT& ar, T const* p, unsigned version, true_t, std::integral_constant<int, 4>)
//{
//	select_archive_construct (ar, p, version);
//}

// Default version for saving `construct' parameters
template<class ArchiveT, class T>
inline void archive_save_construct (ArchiveT& ar, T const* p, unsigned long version)
{
	// by default we call into straight serialization
	archive (ar, *p, version);
}

template<class ArchiveT, class T>
inline void invoke_archive_save_construct(ArchiveT& ar, T* o, unsigned version, std::true_type)
{
	o->archive_save_construct (ar, version);
}

#if 1
	template<class ArchiveT, class T>
inline void invoke_archive_save_construct(ArchiveT& ar, T* o, unsigned version, std::false_type /* non member construct*/)
{
	archive_save_construct (ar, o, version);
}
#endif

	
template<class ArchiveT, class T>
inline void select_archive_save_construct(ArchiveT& ar, T* o, unsigned version)
{
	// save_contruct is selected same as select_archive_construct
	typedef std::integral_constant<bool, has_archive_save_construct<ArchiveT, T>::value> has_memfun_t;
	
	//	typedef T CLASS_DOEST_HAVE_SAVE_CONTRUCT_FUNCTION[has_memfun_t::value ? 1 : -1];
	
	invoke_archive_save_construct (ar, o, version, has_memfun_t());
}


	// Writing an object trough pointer
template<class ArchiveT, class T>
inline void dispatch_archive_impl (ArchiveT& ar, T const* p, unsigned version, false_t, std::integral_constant<int, 4>)
{
	select_archive_save_construct(ar, p, version);
}
	
	
// Reading an object into a pointer (need to create object first)
template<class ArchiveT, class T>
inline void dispatch_archive_impl (ArchiveT& ar, T* const& p, unsigned version, true_t, std::integral_constant<int, 4>)
{
	char* ps = new char[sizeof(T)];
	select_archive_construct(ar, (T*)ps, version);
	const_cast<T*&> (p) = (T*)ps;
}


// T is enum
//template<class ArchiveT, class T>
//inline void dispatch_archive_impl (ArchiveT& ar, T const& v, unsigned version, AnyT, std::integral_constant<int, 5>)
//{
//	if (w.addr != 0 && w.count != 0)
//		ar.read_data ((void*)w.addr, w.count * sizeof(T));
//}
	
template<class ArchiveT, class T>
inline void dispatch_archive_impl (ArchiveT& ar, nv_impl<T> const& p, unsigned version, true_t /*is_reading*/, std::integral_constant<int, 5>)
{
	// TODO: lookup by name?
//	printf("de-serializing %s %s\n", p.name, typeid(p.obj).name());
	ar.object_name (p.name);
	archive (ar, p.obj);
	ar.object_name (NULL);
}
	
template<class ArchiveT, class T>
inline void dispatch_archive_impl (ArchiveT& ar, nv_impl<T> const& p, unsigned version, false_t /*is_reading*/, std::integral_constant<int, 5>)
{
	// if archive supports name-value-pair natively, pass on, otherwise...
	// if we want to save name, do it as separate fields
	// printf("serializing %s %s\n", p.name, typeid(p.obj).name());
	ar.object_name (p.name);
	archive (ar, p.obj);
	ar.object_name (NULL);
}
	
template<class ArchiveT, class T>
inline void archive (ArchiveT& ar, T const& obj)
{
	typedef typename std::remove_cv<T>::type clean_t;
	typedef typename is_nvp<clean_t>::type is_nvp_t;

	typedef
		typename boost::mpl::if_< is_nvp_t, std::integral_constant<int, 5>,
			typename boost::mpl::if_< std::is_array<clean_t>, std::integral_constant<int, 3>,
				typename boost::mpl::if_< std::is_pointer<clean_t>, std::integral_constant<int, 4>,
					typename boost::mpl::if_< std::is_enum<clean_t>, std::integral_constant<int, 0>,
						typename boost::mpl::if_< std::is_fundamental<clean_t>, std::integral_constant<int, 0>,
							typename boost::mpl::if_< is_memblock_t<clean_t>, std::integral_constant<int, 2>,
							std::integral_constant<int, 1> >::type
						>::type
					>::type
				>::type
			>::type
	>::type object_type_t;
	
	typedef std::integral_constant<bool, ArchiveT::is_reading> is_reading_t;
	
	unsigned code_version = version<clean_t>::value;
	dispatch_archive_impl (ar, obj, code_version, is_reading_t(), object_type_t() );
}
	
template<class ArchiveT, bool IsContextCarrier>
	struct reduce_archive_helper { typedef ArchiveT type; };
	
template<class ArchiveT>
	struct reduce_archive_helper<ArchiveT, true> { typedef typename ArchiveT::base_t type; };
	
template<class ArchiveT>
struct reduce_archive
{
	typedef typename reduce_archive_helper<ArchiveT, is_context_carrier<ArchiveT>::value>::type type;
};
	
template<class ArchiveT, class T>
inline typename pulmotor::enable_if<std::is_base_of<pulmotor_archive, ArchiveT>::value, ArchiveT>::type&
operator& (ArchiveT& ar, T const& obj)
{
	archive (ar, obj);
	return ar;
}

template<class ArchiveT, class T, class... Args>
inline typename pulmotor::enable_if<std::is_base_of<pulmotor_archive, ArchiveT>::value, ArchiveT>::type&
operator& (ArchiveT& ar, object_context<T, Args...> const& ctx)
{
	typedef object_context<T, Args...> ctx_t;
	typedef typename reduce_archive<ArchiveT>::type base_t;
	
	archive_with_context<base_t, typename ctx_t::data_t> arch_ctx (ar, ctx.m_data);
	archive (arch_ctx, ctx.m_object);
	return ar;
}
	
template<class ArchiveT, class AsT, class ActualT>
inline typename pulmotor::enable_if<std::is_base_of<pulmotor_archive, ArchiveT>::value, ArchiveT>::type&
operator& (ArchiveT& ar, as_holder<AsT, ActualT> const& ash)
{
	AsT asObj = ash.m_actual;
	archive (ar, asObj);
	ash.m_actual = asObj;
	return ar;
}
	
template<class ArchiveT, class T>
inline typename pulmotor::enable_if<std::is_base_of<pulmotor_archive, ArchiveT>::value, ArchiveT>::type&
operator| (ArchiveT& ar, T const& obj)
{
	archive (ar, obj);
	return ar;
}

template<class ArchiveT, class T, class... Args>
inline typename pulmotor::enable_if<std::is_base_of<pulmotor_archive, ArchiveT>::value, ArchiveT>::type&
operator| (ArchiveT& ar, object_context<T, Args...> const& ctx)
{
	typedef object_context<T, Args...> ctx_t;
	typedef typename reduce_archive<ArchiveT>::type base_t;
	
	archive_with_context<base_t, typename ctx_t::data_t> arch_ctx (ar, ctx.m_data);
	archive (arch_ctx, ctx.m_object);
	return ar;
}
	
template<class ArchiveT, class AsT, class ActualT>
inline typename pulmotor::enable_if<std::is_base_of<pulmotor_archive, ArchiveT>::value, ArchiveT>::type&
operator| (ArchiveT& ar, as_holder<AsT, ActualT> const& ash)
{
	AsT asObj = ash.m_actual;
	archive (ar, asObj);
	ash.m_actual = asObj;
	return ar;
}
	
		
	
/*
template<class ArchiveT, class T>
inline ArchiveT&
	operator| (ArchiveT& ar, typename pulmotor::enable_if<
			   std::is_base_of<pulmotor_archive, ArchiveT>::value &&
			   !std::is_base_of<object_context_tag, T>::value, T const&>::type obj)
{
	archive (ar, obj);
	return ar;
}
	
template<class ArchiveT, class T>
inline ArchiveT&
	operator| (ArchiveT& ar, typename pulmotor::enable_if<
			   std::is_base_of<pulmotor_archive, ArchiveT>::value &&
			   std::is_base_of<object_context_tag, T>::value, T const&>::type obj)
{
	typedef typename reduce_archive<ArchiveT>::type base_t;
	
	typedef typename T::data_t data_t;
	archive_with_context<base_t, data_t> arch_ctx (ar, obj.m_data);
	archive (arch_ctx, obj);
	return ar;
}*/

	

}

#endif
