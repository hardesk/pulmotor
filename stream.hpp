#ifndef PULMOTOR_CONTAINER_HPP_
#define PULMOTOR_CONTAINER_HPP_

#include <memory>
#include <iterator>
#include <boost/noncopyable.hpp>
#include <boost/get_pointer.hpp>

#include "pulmotor_config.hpp"
#include "stream_fwd.hpp"

#ifdef _WINNT
#define pulmotor_native_path(x) L ## x
#else
#define pulmotor_native_path(x) x
#endif

namespace pulmotor
{
	enum error_id
	{
		k_ok,
		k_eof,
		k_read_fail,
		k_write_fail,
		k_out_of_space
	};

	PULMOTOR_ATTR_DLL wchar_t const* get_error_id_text( error_id id );

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
	class basic_version
	{
		static const unsigned MA_MASK = 0xffff0000;
		static const unsigned MI_MASK = 0x0000ffff;

	public:
		basic_version( unsigned v = 0x0 )
		:	ver( v )
		{}

		unsigned major() const	{ return (ver & MA_MASK) >> 16; }
		unsigned minor() const	{ return ver & MI_MASK; }
		bool operator==( basic_version const& v ) const	{	return ver == v.ver; }
		bool operator<( basic_version const& v ) const	{	return ver < v.ver; }

	private:
        unsigned	ver;	
	};

	//
	class PULMOTOR_ATTR_DLL cfile_output_buffer : public basic_output_buffer
	{
	public:
		cfile_output_buffer (pulmotor::string const& file_name);
		virtual ~cfile_output_buffer();

		virtual error_id write (void const* src, size_t count, size_t* was_written);
		
		bool good () const {
			return file_ != 0;
		}

	private:
		string	name_;
		FILE*	file_;
	};

	//
	class PULMOTOR_ATTR_DLL cfile_input_buffer : public basic_input_buffer
	{
	public:
		cfile_input_buffer (pulmotor::string const& file_name);
		virtual ~cfile_input_buffer();

		virtual error_id read (void* dest, size_t count, size_t* was_read);

	private:
		string	name_;
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
		static const int magic_chars = 0x08;
		static const char pulmotor_magic_text[magic_chars];

	public:
		static const size_t max_desc = 0x10;
	
		enum flags_enum
		{
			flag_plain	= 0x00,
			flag_zlib	= 0x01,
			flag_bzip2	= 0x02,
			flag_lzma	= 0x04
		};

		static char const* get_pulmotor_magic()
		{	return pulmotor_magic_text; }

		basic_header()
		{
			char const* m = "pulmotor";
			std::copy( m, m + magic_chars, magic );
		}

		basic_header( basic_version const& ver, char const* desc, int flags_ )
			:	version( ver ), flags( flags_ )
		{
			char const* m = "pulmotor";
			std::copy( m, m + magic_chars, magic );
			size_t desclen = (std::min) (max_desc - 1, strlen(desc));
			std::copy (desc, desc + desclen, description);
			std::fill_n (description + desclen, max_desc - desclen, 0);
		}

		char const* get_magic() const	{	return magic; }
		int get_magic_length() const	{	return magic_chars; }

		basic_version const& get_version() const	{ return version; }
		char const* get_description() const	{ return description; }

		int get_flags() const	{	return flags; }

	private:
        char			magic[magic_chars];
		basic_version	version;
		int				flags;
		char			description [max_desc];
	};

	// global input/output factory functions
	std::auto_ptr<basic_input_buffer> PULMOTOR_ATTR_DLL create_input (pulmotor::string const& file_name);

	std::auto_ptr<basic_input_buffer> PULMOTOR_ATTR_DLL create_plain_input (pulmotor::string const& file_name);
	std::auto_ptr<basic_output_buffer> PULMOTOR_ATTR_DLL create_plain_output( pulmotor::string const& file_name );
}

#endif // PULMOTOR_CONTAINER_HPP_
