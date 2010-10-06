#include "stream.hpp"
#include <sys/stat.h>
#include <errno.h>

namespace pulmotor
{
	
#ifdef _DEBUG
void checkerrno() {
	if (errno != 0)
		printf("errno: %d\n", errno);
}
#else
	inline void checkerrno() {}
#endif

const char basic_header::pulmotor_magic_text[basic_header::magic_chars]
= { 'p', 'u', 'l', 'M' };

static char const* error_desc [k_error_id_count] =
{
		"ok", "eof", "read-fail", "write-fail", "out-of-space"
};

char const* get_error_id_text( error_id id )
{
	return error_desc [id];
}
	
basic_input_buffer::~basic_input_buffer()
{}

basic_output_buffer::~basic_output_buffer()
{}

//
cfile_output_buffer::cfile_output_buffer (pulmotor::pp_char const* file_name)
#ifdef _DEBUG
	:	name_ (file_name)
#endif
{
#ifdef _WIN32
	file_ = _wfopen (file_name, L"wb+");
#else
	file_ = fopen (file_name, "wb+");
#endif
}

cfile_output_buffer::~cfile_output_buffer ()
{
	if (file_)
		fclose (file_);
}

error_id cfile_output_buffer::write (void const* src, size_t count, size_t* was_written)
{
	size_t res = fwrite (src, 1, count, file_);
	if (was_written)
		*was_written = res;
	return k_ok;
}

cfile_input_buffer::cfile_input_buffer (pulmotor::pp_char const* file_name)
#ifdef _DEBUG
	:	name_ (file_name)
#endif
{
#ifdef _WIN32
	file_ = _wfopen (file_name, L"rb");
#else
	file_ = fopen (file_name, "rb");
#endif
}

cfile_input_buffer::~cfile_input_buffer ()
{
	if (file_)
		fclose (file_);
}

error_id cfile_input_buffer::read (void* dest, size_t count, size_t* was_read)
{
	size_t res = fread (dest, 1, count, file_);
	
	if (was_read)
		*was_read = res;
	
	return k_ok;
}

/*
//
native_file_output::native_file_output( pulmotor::string const& fileName )
{
	m_handle = ::CreateFile(
		fileName.c_str(),
		GENERIC_WRITE,
		FILE_SHARE_READ,
		0,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);

	if( m_handle == INVALID_HANDLE_VALUE )
	{
//		BOOST_LOG( stir_error ) << "Failed to open file: " << to_ascii(fileName) << std::endl;
		return;
	}
}

native_file_output::~native_file_output()
{
	if( is_valid() )
	{
		if( !CloseHandle( m_handle ) )
			;
//			BOOST_LOG( stir_error ) << "Failed to close file: " << m_handle << std::endl;
	}
}

size_t native_file_output::putcn( char const* src, size_t count )
{
	DWORD written = 0;
	if( !::WriteFile( m_handle, src, (DWORD)count, &written, 0 ) )
	{
//		BOOST_LOG( stir_error ) << "Failed to write to file: " << m_handle << ", amount: " << count << std::endl;
	}
	return written;
}*/


std::auto_ptr<basic_input_buffer> create_plain_input (pulmotor::pp_char const* file_name)
{
	std::auto_ptr<basic_input_buffer> ptr;
	
	ptr.reset (new cfile_input_buffer (file_name));

	return ptr;
}

std::auto_ptr<basic_output_buffer> create_plain_output (pulmotor::pp_char const* file_name)
{
	std::auto_ptr<basic_output_buffer> ptr;
	
	ptr.reset (new cfile_output_buffer(file_name));
	
	return ptr;
}

file_size_t get_file_size (pulmotor::pp_char const* file_name)
{
	struct stat s;
	if (stat (file_name, &s) == 0)
		return s.st_size;
	
	checkerrno ();
	
	return 0;
}
	
}
