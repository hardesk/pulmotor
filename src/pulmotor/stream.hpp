#ifndef PULMOTOR_CONTAINER_HPP_
#define PULMOTOR_CONTAINER_HPP_

#include <memory>
#include <iterator>
#include <boost/noncopyable.hpp>
#include <boost/get_pointer.hpp>

#include "pulmotor_config.hpp"
#include "stream_fwd.hpp"
#include "util.hpp"

#if PULMOTOR_STIR_PATH_SUPPORT
#include <stir/path.hpp>
#endif

#ifdef _WINNT
#define pulmotor_native_path(x) L ## x
#else
#define pulmotor_native_path(x) x
#endif

namespace pulmotor
{

bool do_test();

enum class error
{
	ok,
	eof,
	read_fail,
	write_fail,
	out_of_space,

	count
};

struct endian
{
	union tester {
		unsigned u;
		char c[2];
	};

	static bool is_le() { return true; } // tester x; x.u=0x01; return x.c[0]==1; }
	static bool is_be() { return false; } // tester x; x.u=0x01; return x.c[0]==0; }
};

PULMOTOR_ATTR_DLL char const* error_to_text( error code );
PULMOTOR_ATTR_DLL int get_pagesize();

class block_common
{
	block_common(block_common const& a) = delete;
	block_common& operator=(block_common const& a) = delete;

protected:
	char* m_data;
	fs_t m_bloff; // block offset in stream
	size_t m_blsize, m_cur;

	void zero() { m_data=nullptr; m_bloff=0; m_blsize=0; m_cur=0; }

public:
	block_common() { zero(); }
	fs_t offset() const { return m_bloff + m_cur; }
	size_t avail() { return m_blsize - m_cur; }
	char* data() { return m_data + m_cur; }
};

class source : public block_common
{
protected:
	virtual void make_available(std::error_code& ec) = 0;
public:
	void advance(size_t sz, std::error_code& ec);
	size_t fetch(void* dest, size_t sz, std::error_code& ec);

	virtual fs_t size() = 0;
};

class source_mmap : public source
{
public:
	enum flags { ro, wr, rw };

private:
	void* m_mmap;
	int m_fd;
	size_t m_mmapblock;
	fs_t m_filesize;
	flags m_flags;

	void reset()
	{
		m_mmap=nullptr;
		m_fd=-1;
		m_mmapblock=0;
		m_filesize=0;
	}

	virtual void make_available(std::error_code& ec);

public:
	source_mmap();
	~source_mmap();

	void map(path_char const* path, flags fl, std::error_code& ec, fs_t off = 0, size_t mmap_block_size = 0);
	void unmap(std::error_code& ec);

	virtual fs_t size();
};

class source_istream : public source
{
	size_t m_cache_size;
	fs_t m_file_size;
	std::istream& m_stream;

	virtual void make_available(std::error_code& ec);

public:
	source_istream(std::istream& s, size_t cache_size = 0);
	~source_istream();

	void prefetch();

	virtual fs_t size();
};

class sink
{
public:
	virtual void put(char const* data, size_t size) = 0;
};

class sink_ostream : public sink
{
	std::ostream& m_stream;

public:
	sink_ostream(std::ostream& os);
	~sink_ostream();

	void put(char const* data, size_t size);
};


	// buffer -> [coder] -> formatter
	// formatter <- [coder] <- buffer
	
	//
	class PULMOTOR_ATTR_DLL basic_input_buffer
	{
	public:
		virtual ~basic_input_buffer() = 0;
		virtual size_t read (void* dest, size_t count, std::error_code& ec) = 0;
	};

	//
	class PULMOTOR_ATTR_DLL basic_output_buffer : boost::noncopyable
	{
	public:
		virtual ~basic_output_buffer() = 0;
		virtual size_t write (void const* src, size_t count, std::error_code& ec) = 0;
	};
	
	//
	template<class T>
	class basic_version
	{
		enum { bits = sizeof(T) << 3 };
		static const unsigned MA_MASK = (1 << bits/2) - 1 << bits/2;
		static const unsigned MI_MASK = (1 << bits/2) - 1;

	public:
		basic_version( unsigned v = 0x0 )
		:	ver( v )
		{}

		unsigned major() const	{ return (ver & MA_MASK) >> bits/2; }
		unsigned minor() const	{ return ver & MI_MASK; }
		bool operator==( basic_version const& v ) const	{	return ver == v.ver; }
		bool operator<( basic_version const& v ) const	{	return ver < v.ver; }

		void change_endianess ()
		{
			util::swap_variable (ver);
		}

	private:
        T	ver;
	};

	//
	class PULMOTOR_ATTR_DLL cfile_output_buffer : public basic_output_buffer
	{
	public:
		cfile_output_buffer (pulmotor::path_char const* file_name, std::error_code& ec);
		virtual ~cfile_output_buffer();

		virtual size_t write (void const* src, size_t count, std::error_code& ec);
		
		bool good () const {
			return file_ != 0;
		}
		
		FILE* handle() { return file_; }

	private:
#ifdef _DEBUG
		string	name_;
#endif
		FILE*	file_;
	};

	//
	class PULMOTOR_ATTR_DLL cfile_input_buffer : public basic_input_buffer
	{
	public:
		cfile_input_buffer (pulmotor::path_char const* file_name, std::error_code& ec);
		virtual ~cfile_input_buffer();

		virtual size_t read (void* dest, size_t count, std::error_code& ec);
		bool good () const {
			return file_ != 0;
		}

	private:
#ifdef _DEBUG
		string	name_;
#endif
		FILE*	file_;
	};


/*	// 
	class native_file_output : public basic_output_buffer
	{
	public:
		native_file_output( pulmotor::string const& file_name );
		virtual ~native_file_output();

		virtual error_id putcn( char const* src, size_t count, size_t* was_written );

		bool is_valid() const { return m_handle != INVALID_HANDLE_VALUE; }

	private:
		HANDLE			m_handle;
	};*/

	class PULMOTOR_ATTR_DLL basic_header
	{
		static const int magic_chars = 0x04;
		static const char pulmotor_magic_text[magic_chars];

	public:
		enum flags_enum
		{
			flag_plain	= 0x0000,
			flag_zlib	= 0x0001,
			flag_bzip2	= 0x0002,
			flag_lzma	= 0x0003,
			flag_be		= 0x0100,
			flag_checksum=0x0200,
		};

		static char const* get_pulmotor_magic()
		{	return pulmotor_magic_text; }

		basic_header()
		{
			std::copy( pulmotor_magic_text, pulmotor_magic_text + magic_chars, magic );
		}

		basic_header( basic_version<unsigned short> const& ver, int flags_ = flag_plain )
			:	version( ver ), flags( flags_ )
		{
			std::copy( pulmotor_magic_text, pulmotor_magic_text + magic_chars, magic );
//			size_t desclen = (std::min) (max_desc - 1, strlen(desc));
//			std::copy (desc, desc + desclen, description);
//			std::fill_n (description + desclen, max_desc - desclen, 0);
		}

		char const* get_magic() const	{ return magic; }
		int get_magic_length() const	{ return magic_chars; }

		basic_version<unsigned short> const& get_version() const	{ return version; }
		unsigned get_flags() const	{	return flags; }
		basic_header& set_flag (unsigned flag) { flags |= flag; return *this; }

		bool is_littleendian () const { return (flags & flag_be) == 0; }
		bool is_bigendian () const { return (flags & flag_be) != 0; }

		void change_endianess ()
		{
			version.change_endianess ();
			util::swap_variable (flags);
		}

	private:
        char							magic [magic_chars];
		basic_version<unsigned short>	version;
		unsigned short					flags;
	};

	// global input/output factory functions
	fs_t file_size (pulmotor::path_char const* file_name, std::error_code& ec);
	inline fs_t file_size (pulmotor::path_char const* file_name) { std::error_code ec; fs_t s = file_size(file_name, ec); return s; }
	std::unique_ptr<basic_input_buffer> PULMOTOR_ATTR_DLL create_plain_input (pulmotor::path_char const* file_name);
	std::unique_ptr<basic_output_buffer> PULMOTOR_ATTR_DLL create_plain_output(pulmotor::path_char const* file_name);	
	inline std::unique_ptr<basic_input_buffer> PULMOTOR_ATTR_DLL create_input (pulmotor::path_char const* file_name) { return create_plain_input (file_name); }
	
#if PULMOTOR_STIR_PATH_SUPPORT
	// global input/output factory functions
	inline fs_t get_file_size (stir::path const& file_name)
	{ return get_file_size (file_name.c_str()); }
	inline std::unique_ptr<basic_input_buffer> PULMOTOR_ATTR_DLL create_plain_input (stir::path const& file_name)
	{ return create_plain_input (file_name.c_str()); }
	inline std::unique_ptr<basic_output_buffer> PULMOTOR_ATTR_DLL create_plain_output(stir::path const& file_name)
	{ return create_plain_output (file_name.c_str()); }
	inline std::unique_ptr<basic_input_buffer> PULMOTOR_ATTR_DLL create_input (stir::path const& file_name)
	{ return create_plain_input (file_name.c_str()); }
#endif
}

#endif // PULMOTOR_CONTAINER_HPP_