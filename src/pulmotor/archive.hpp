#ifndef PULMOTOR_ARCHIVE_HPP
#define PULMOTOR_ARCHIVE_HPP

#include "pulmotor_config.hpp"
#include "pulmotor_types.hpp"

#include <type_traits>

#include <cstdarg>
#include <cstddef>
#include <cassert>
#include <vector>
#include <algorithm>
#include <typeinfo>
#include <string>
#include <unordered_map>

#include "stream.hpp"
#include "util.hpp"

//#include <stir/filesystem.hpp>

#define PULMOTOR_SERIALIZE_ASSERT(xxx, msg) static_assert(xxx, msg)
#define PULMOTOR_RUNTIME_ASSERT(xxx, msg) assert(xxx && msg)
#define PULMOTOR_DEBUG_ARCHIVE 0

namespace pulmotor {

enum
{
	align_natural = 0
};


struct serialize_context_tag {};

template<class T, class... D>
struct serialize_context : public serialize_context_tag
{
	typedef std::tuple<D...> data_t;

	serialize_context(T& o, D&&... data) : m_object(o), m_data(std::forward<D>(data)...) { }

	data_t m_data;
	T& m_object;
};

struct archive
{
	archive() {}
	~archive() {}

	archive(archive const&) = delete;
	archive& operator=(archive const&) = delete;

	void begin_array () {}
	void end_array () {}
	void begin_object () {}
	void end_object () {}

	void object_name(char const*) {}
};

template<class T>
struct is_archive_if : std::enable_if<std::is_base_of<archive, T>::value, T> {};

extern char null_32[32];

template<class Derived>
struct archive_write_util
{
	enum { is_stream_aligned = true };

	Derived& self() { return *static_cast<Derived*>(this); }

	void align_stream(size_t al) {
		while(1) {
			fs_t offset = self().offset();
			if ((offset & (al-1)) != 0) {
				size_t write = util::align(offset, al) - offset;
				assert(write <= 32);
				self().write_data(null_32, write < 32 ? write : 32);
			} else
				break;
		}
	}
};

template<class Derived>
struct archive_write_version_util
{
	enum { forced_align = 256 };

	//unsigned version_flags() { }
	archive_write_version_util(unsigned flags) : m_flags(flags) {}

	Derived& self() { return *static_cast<Derived*>(this); }

	template<class T>
	void write_basic_aligned(T& o, unsigned align = 0) {
		self().align_stream(align == 0 ? sizeof o : align);
		self().write_data(&o, sizeof o);
	}

	template<class T>
	void write_object_prefix(T const* obj, unsigned version) {
		assert(version != no_version);
		u32 vf = version;

		if (obj == nullptr)
			vf |= ver_flag_null_ptr;

		fs_t block_size = 0;
		if ((m_flags & ver_flag_align_object)) {// || (m_flags & ver_flag_debug_string)) {
			vf |= ver_flag_garbage_length;
		}

		std::string name;
		u32 name_length = 0;
		if (m_flags & ver_flag_debug_string) {
			vf |= ver_flag_debug_string;

			name = typeid(T).name();
			name_length = name.size();
			if (name_length > ver_debug_string_max_size) name_length = ver_debug_string_max_size;
			block_size += sizeof name_length + name_length;
		}

		u32 garbage_len = block_size;
		if (m_flags & ver_flag_align_object) {
			vf |= ver_flag_align_object;
			block_size += sizeof garbage_len;
		}

		// write version, then calculate how much we'll actually write afterwards
		self().align_stream(sizeof vf);
		self().write_basic(vf);

		// [version] [garbage_length]? ([string-length] [string-data)? [ ... alignment-data ... ]? [object]
		fs_t base = self().offset();

		fs_t objs = 0;
		if(vf & ver_flag_align_object) {
			objs = util::align(base + block_size, forced_align);
			garbage_len = objs - base - sizeof garbage_len;
		}

		if (vf & ver_flag_garbage_length) {
			self().write_basic(garbage_len);
		}

		if (vf & ver_flag_debug_string) {
			self().write_basic(name_length);
			self().write_data(name.data(), name_length);
		}

		if (vf & ver_flag_align_object) {
			assert(objs >= self().offset());
			size_t align_size = objs - self().offset();
#if PULMOTOR_DEBUG_ARCHIVE
			for (size_t i=align_size; i --> 0; )
				self().write_basic((u8)(i>0 ? '-' : '>'));
#else
			for (size_t i=align_size; i>0; ) {
				size_t count = i > 32 ? 32 : i;
				self().write_data(null_32,  count);
				i -= count;
			}
#endif
			assert(self().offset() == util::align(self().offset(), forced_align) && "garbage alignment is not correct");
		}
	}

private:
	unsigned m_flags;
};

struct object_version
{
	unsigned v;

	object_version& operator=(object_version const&) = default;

	unsigned version() const { return v & ver_flag_mask; }
	unsigned is_nullptr() const { return v & ver_flag_null_ptr; }

	operator unsigned() const { return v & ver_flag_mask; }
};

template<class Derived>
struct archive_read_util
{
	enum { is_stream_aligned = true };
	Derived& self() { return *static_cast<Derived*>(this); }

	template<class T>
	void read_basic_aligned(T& o, unsigned align = 0) {
		align_stream(align == 0 ? sizeof o : align);
		self().read_basic(o);
	}

	void align_stream(size_t al) {
		fs_t offset = self().offset();
		if ((offset & (al-1)) != 0)
			self().advance(util::align(offset, al) - offset);
	}

	object_version process_prefix() {

		u32 version=0, garbage=0, debug_string_len=0;
		self().read_basic(version);

		if (version & ver_flag_garbage_length) {
			self().read_basic(garbage);
			self().advance(garbage);
		} else {
			if (version & ver_flag_debug_string) {
				self().read_basic(debug_string_len);
				self().advance(debug_string_len);
			}
		}

		return object_version{version};
	}
};

template<class Derived>
struct archive_pointer_support
{
	enum { track_pointers = true };
	Derived& self() { return *static_cast<Derived*>(this); }

	std::unordered_map<void*, u32> m_ptrtoid;
	std::unordered_map<u32, void*> m_idtoptr;

	void restore_ptr(u32 id, void* obj) {
		assert(m_idtoptr.find(id) == m_idtoptr.end());;

		m_idtoptr.insert(std::make_pair(id, obj));
		m_ptrtoid.insert(std::make_pair(obj, id));
	}
	u32 reg_ptr(void* obj) {
		auto it = m_ptrtoid.find(obj);
		if (it != m_ptrtoid.end())
			return it->second;

		u32 id = m_ptrtoid.size() + 1;
		m_idtoptr.insert(std::make_pair(id, obj));
		m_ptrtoid.insert(std::make_pair(obj, id));
		return id;
	}

	u32 lookup_id(void* ptr) {
		if (auto it = m_ptrtoid.find(ptr); it != m_ptrtoid.end())
			return it->second;
		return 0;
	}
	void* lookup_ptr(u32 id) {
		if (auto it = m_idtoptr.find(id); it != m_idtoptr.end())
			return it->second;
		return nullptr;
	}
};

struct archive_whole : archive, archive_read_util<archive_whole>
{
	archive_whole (source& s) : source_ (s) {}
	fs_t offset() const { return source_.offset(); }

	enum { is_reading = 1, is_writing = 0 };

	void advance(size_t s)
	{
		assert(source_.offset() + s <= source_.avail());
		source_.advance(s, ec_);
	}

	template<class T>
	void read_basic(T& data) {
		assert( source_.offset() + sizeof(T) <= source_.avail());
		assert( ((uintptr_t)source_.data() & (sizeof(T)-1)) == 0 && "stream alignment issues");
		data = *reinterpret_cast<T*>(source_.data());
		source_.advance(sizeof(T), ec_);
	}

	void read_data (void* dest, size_t size)
	{
		assert( source_.offset() + size <= source_.avail());
		memcpy( dest, source_.data(), size);
		source_.advance( size, ec_ );
	}

	std::error_code ec_;

private:
	source& source_;
};

struct archive_chunked : archive, archive_read_util<archive_chunked>
{
	archive_chunked (source& s) : source_ (s) {}
	fs_t offset() const { return source_.offset(); }

	enum { is_reading = 1, is_writing = 0 };

	void advance(size_t s)
	{
		assert(source_.offset() + s <= source_.size());
		source_.advance(s, ec_);
	}

	template<class T>
	void read_basic(T& data) {
		assert( source_.offset() + sizeof(T) <= source_.size());
		assert( ((uintptr_t)source_.data() & (sizeof(T)-1)) == 0 && "stream alignment issues");
		source_.fetch( &data, sizeof data, ec_ );
	}

	void read_data (void* dest, size_t size)
	{
		assert( source_.offset() + size <= source_.size());
		source_.fetch( dest, size, ec_ );
	}

	std::error_code ec_;

private:
	source& source_;
};

#if 0

template<int FlagsT = 0, class ArchiveT, class T>
void archive (ArchiveT& ar, T const& obj);

template<class ArchiveT, class T>
void archive_construct (ArchiveT& ar, T* p, unsigned long version);

template<class ArchiveT, class T>
void select_archive_construct(ArchiveT& ar, T* o, unsigned version);

template<class T, class... DataT>
object_context<T, DataT...> with_ctx (T& o, DataT&&... data)
{
	return object_context<T, DataT...> (o, std::forward<DataT> (data)...);
}

template<class BaseT, class CtxT>
class archive_with_context : public archive, object_context_tag
{
	typedef CtxT context_t;

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

	template<class T>
	void do_version (version_t& ver) { m_base.template do_version<T>(ver); }

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

class input_archive : public archive
{
	basic_input_buffer& buffer_;
	size_t offset_;

public:
	input_archive (basic_input_buffer& buf) : buffer_ (buf), offset_(0) {}

	size_t offset() const { return offset_; }

	enum { is_reading = 1, is_writing = 0 };
	std::error_code ec;

	template<class T>
	void do_version (version_t& ver) {
		if ((int)ver != pulmotor::version_dont_track)
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

class output_archive : public archive
{
	basic_output_buffer& buffer_;
	size_t written_;

public:
	output_archive (basic_output_buffer& buf) : buffer_ (buf), written_ (0) {}

	enum { is_reading = 0, is_writing = 1 };

	size_t offset() const { return written_; }

	std::error_code ec;

	template<class T>
	void do_version (version_t& ver) {
		if ((int)ver != pulmotor::version_dont_track)
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
class debug_archive : public archive
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

	template<class T>
	void do_version (version_t& ver) {
		size_t off = offset();

		int codeVer = (int)ver;

		m_actual_arch.template do_version<T> (ver);

		std::string name = readable_name<T> ();
		if (codeVer != pulmotor::version_dont_track)
			print_name (("__version '" + name + "'").c_str(), ver, off);
		else
			print_name (("not tracking version for '" + name + "'").c_str(), (int)get_version<T>::value, off);
	}

	template<class T>
	void print_name(char const* name, T const& data, size_t off)
	{
		char buf[128];
		snprintf (buf, sizeof(buf), "%*s0x%08x %s (%s)", m_indent*2, "",
				  (int)off,
				  name ? name : "",
				  readable_name<T> ().c_str()
//				  short_type_name<typename std::remove_cv<T>::type>::name
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
				  (int)offset(), (int)size, size / 1024.0f);
		m_out << buf;

		m_actual_arch.read_data (src, size);
	}
};
#endif

struct archive_sink
	: public archive
	, public archive_write_util<archive_sink>
	, public archive_write_version_util<archive_sink>
	, public archive_pointer_support<archive_sink>
{
	sink& sink_;
	fs_t written_;

public:
	archive_sink (sink& s, unsigned flags = 0)
		: archive_write_version_util<archive_sink>(flags)
		, sink_ (s)
		, written_ (0)
	{}

	enum { is_reading = 0, is_writing = 1 };

	fs_t offset() const { return written_; }

	std::error_code ec;

	template<class T>
	void write_basic (T const& data)
	{
		sink_.write(&data, sizeof(data), ec);
	}

	void write_data (void const* src, size_t size)
	{
		sink_.write (src, size, ec);
		written_ += size;
	}
};

struct archive_vector_out
	: public archive
	, public archive_write_util<archive_vector_out>
	, public archive_write_version_util<archive_vector_out>
	, public archive_pointer_support<archive_vector_out>
{
	std::vector<char> data;

	archive_vector_out(unsigned version_flags = 0) : archive_write_version_util<archive_vector_out>(version_flags)
	{}

	enum { is_reading = false, is_writing = true };

	size_t offset() const { return data.size(); }

	template<class T>
	void write_basic(T const& a) {
		char const* p = (char const*)&a;
		data.insert(data.end(), p, p + sizeof(a));
	}

	void write_data(void* src, size_t size) {
		data.insert(data.end(), (char const*)src, (char const*)src + size);
	}

	std::string str() const { return std::string(data.data(), data.size()); }
};

struct archive_vector_in
	: public archive
	, public archive_read_util<archive_vector_in>
	, public archive_pointer_support<archive_vector_in>
{
	std::vector<char> data;
	size_t m_offset = 0;
	archive_vector_in(std::vector<char> const& i) : data(i) {}

	enum { is_reading = true, is_writing = false };

	size_t offset() const { return m_offset; }

	template<class T>
	void read_basic(T& a) {
		a = *(T const*)(data.data() + m_offset);
		m_offset += sizeof a;
	}

	void advance(size_t s) { m_offset += s; }
	void read_data(void* src, size_t size) { memcpy(src, data.data() + m_offset, size); }

	std::string str() const { return std::string(data.data(), data.size()); }
};

template<class T>
size_t load_archive (path_char const* pathname, T&& obj, std::error_code& ec)
{
	source_mmap s (pathname, source_mmap::ro, ec);
	if (ec)
		return 0U;

	archive_whole ar(s);
	ar | std::forward<T>(obj);
	ec = ar.ec_;

	return ar.offset();
}

/*
template<class T>
size_t load_archive_debug (stir::path const& pathname, T& obj, std::error_code& ec, bool doDebug)
{
	stir::filesystem::input_buffer in (pathname, ec);
	if (ec)
		return 0;

	pulmotor::input_archive inA (in);
	if (doDebug)
	{
		pulmotor::debug_archive<pulmotor::input_archive> inAD(inA, std::cout);
		pulmotor::archive (inAD, obj);
	}
	else
		pulmotor::archive (inA, obj);

	ec = inA.ec;

	return inA.offset();
}*/

template<class T>
size_t save_archive (path_char const* pathname, T&& obj, std::error_code& ec)
{
	std::fstream os(pathname, std::ios_base::binary|std::ios_base::out);
	if (os.good()) {
		ec = std::make_error_code(std::errc::invalid_argument);
		return 0;
	}

	sink_ostream s(os);
	archive_sink ar(s);

	ar | std::forward<T>(obj);

	return ar.offset();
}

/*
template<class T>
size_t save_archive_debug (stir::path const& pathname, T& obj, std::error_code& ec, bool doDebug)
{
	stir::filesystem::output_buffer out (pathname, ec);
	if (ec)
		return 0;

	pulmotor::output_archive outA (out);
	if (doDebug)
	{
		pulmotor::debug_archive<pulmotor::output_archive> outAD(outA, std::cout);
		pulmotor::archive (outAD, obj);
	}
	else
		pulmotor::archive (outA, obj);

	ec = outA.ec;

	return outA.offset();
}*/

} // pulmotor

#endif
