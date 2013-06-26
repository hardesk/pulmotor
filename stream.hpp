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
	typedef u32 file_size_t;
	
	enum error_id
	{
		k_ok,
		k_eof,
		k_read_fail,
		k_write_fail,
		k_out_of_space,

		k_error_id_count // number of error codes
	};

	inline bool is_le ()
	{
		unsigned value = 0x01;
		return *(char const*)&value == 0x01;
	}

	inline bool is_be ()
	{
		unsigned value = 0x01;
		return *(char const*)&value == 0x00;
	}
	

	PULMOTOR_ATTR_DLL char const* get_error_id_text( error_id id );

	// buffer -> [coder] -> formatter
	// formatter <- [coder] <- buffer

	//
	class PULMOTOR_ATTR_DLL basic_input_buffer : boost::noncopyable
	{
	public:
		virtual ~basic_input_buffer() = 0;
		virtual error_id read (void* dest, size_t count, size_t* was_read) = 0;
	};

	//
	class PULMOTOR_ATTR_DLL basic_output_buffer : boost::noncopyable
	{
	public:
		virtual ~basic_output_buffer() = 0;
		virtual error_id write (void const* src, size_t count, size_t* was_written) = 0;
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
		cfile_output_buffer (pulmotor::pp_char const* file_name);
		virtual ~cfile_output_buffer();

		virtual error_id write (void const* src, size_t count, size_t* was_written);
		
		bool good () const {
			return file_ != 0;
		}

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
		cfile_input_buffer (pulmotor::pp_char const* file_name);
		virtual ~cfile_input_buffer();

		virtual error_id read (void* dest, size_t count, size_t* was_read);
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
	file_size_t get_file_size (pulmotor::pp_char const* file_name);
	std::auto_ptr<basic_input_buffer> PULMOTOR_ATTR_DLL create_plain_input (pulmotor::pp_char const* file_name);
	std::auto_ptr<basic_output_buffer> PULMOTOR_ATTR_DLL create_plain_output(pulmotor::pp_char const* file_name);	
	inline std::auto_ptr<basic_input_buffer> PULMOTOR_ATTR_DLL create_input (pulmotor::pp_char const* file_name) { return create_plain_input (file_name); }
	
#if PULMOTOR_STIR_PATH_SUPPORT
	// global input/output factory functions
	inline file_size_t get_file_size (stir::path const& file_name)
	{ return get_file_size (file_name.c_str()); }
	inline std::auto_ptr<basic_input_buffer> PULMOTOR_ATTR_DLL create_plain_input (stir::path const& file_name)
	{ return create_plain_input (file_name.c_str()); }
	inline std::auto_ptr<basic_output_buffer> PULMOTOR_ATTR_DLL create_plain_output(stir::path const& file_name)
	{ return create_plain_output (file_name.c_str()); }
	inline std::auto_ptr<basic_input_buffer> PULMOTOR_ATTR_DLL create_input (stir::path const& file_name)
	{ return create_plain_input (file_name.c_str()); }
#endif
}

#endif // PULMOTOR_CONTAINER_HPP_
