#include "stream.hpp"
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <istream>


// for testing, delete later
#include <sstream>
#include <fstream>

namespace pulmotor
{

int get_pagesize()
{
	static int ps = 0;
	if (!ps) {
#ifdef _WIN32
		SYSTEM_INFO si;
		GetSystemInfo(&si);
		ps = si.dwPageSize;
#elif defined(__APPLE__)
		ps = getpagesize();
#endif
	}
	return ps;
}

void source::advance(size_t sz, std::error_code& ec) {
	while(1) {
		if ((m_cur += sz) > m_blsize) {
			make_available(ec); 
			if (ec) break;
		} else
			break;
	}
}

size_t source::fetch(void* dest, size_t sz, std::error_code& ec) {
	size_t copied = 0;
	while(1) {
		size_t left = m_blsize - m_cur;
		if (left == 0) {
			make_available(ec); 
			if (ec || offset() >= size()) break;
			left = m_blsize - m_cur;
		}
		size_t capped = sz < left ? sz : left;
		   
		memcpy(dest, m_data + m_cur, capped);
		copied += capped;

		if ((m_cur += capped) >= m_blsize) {
			make_available(ec); 
			if (ec) break;
			//if (offset() >= size()) break;
			if (m_cur >= m_blsize) break;
		}

		if ((sz -= capped) == 0) break;
	}
	return copied;
}

sink_ostream::sink_ostream(std::ostream& os) : m_stream(os)
{
}

sink_ostream::~sink_ostream()
{
	m_stream.flush();
}

void sink_ostream::write(void const* data, size_t size, std::error_code& ec)
{
	m_stream.write((char const*)data, size);
}

inline std::error_code mk_ec(int err) { return std::make_error_code((std::errc)err); }

source_mmap::source_mmap()
{
	reset();
}

static int get_opfl(source_mmap::flags fl) { return fl==source_mmap::ro ? O_RDONLY : fl==source_mmap::wr ? O_WRONLY : O_RDWR; }
static int get_protfl(source_mmap::flags fl) { return fl==source_mmap::ro ? PROT_READ : fl==source_mmap::wr ? PROT_WRITE : (PROT_READ|PROT_WRITE); }

source_mmap::~source_mmap()
{
	if (m_mmap != nullptr)
		if (munmap(m_mmap, m_blsize) == -1)
			assert(!"~source_mmap: munmap normally should succeed");

	if (m_fd != -1)
		close(m_fd);
}

void source_mmap::map(path_char const* path, flags fl, std::error_code& ec, fs_t off, size_t mmap_block_size)
{
	m_flags = fl;
	if(off % get_pagesize() != 0 || mmap_block_size % get_pagesize() != 0) {
		ec = std::make_error_code(std::errc::invalid_argument);
		return;
	}

	// ec.clear();
	int opfl=get_opfl(fl);
	if ((m_fd = open(path, opfl, 0)) == -1) {
		ec=mk_ec(errno);
		return;
	}

	struct stat st;
	if (fstat(m_fd, &st) == -1) {
		ec=mk_ec(errno);
		return;
	}
	m_bloff=off;

	m_filesize=st.st_size;
	size_t blsz=mmap_block_size;
	if ((m_mmapblock=mmap_block_size) == 0)
		blsz=m_filesize;

	int protfl=get_protfl(fl);
	if ((m_mmap = m_data = (char*)mmap(NULL, blsz, protfl, MAP_SHARED, m_fd, m_bloff)) == MAP_FAILED) {
		ec=mk_ec(errno);
		close(m_fd);
		reset();
		zero();
		return;
	}
	m_blsize=blsz;
}

void source_mmap::unmap(std::error_code& ec)
{
	// ec.clear();
	if (m_fd != -1) {
		if (munmap(m_mmap, m_blsize) == -1)
			ec=mk_ec(errno);
		if (close(m_fd)!=0 && !ec)
			ec=mk_ec(errno);
		reset();
		zero();
	} else
		ec=std::make_error_code(std::errc::invalid_argument);
}

void source_mmap::make_available(std::error_code& ec)
{
	size_t ps = get_pagesize();
	fs_t foff = m_bloff + m_cur;
	size_t off = foff & (ps - 1);
	size_t base = foff & ~(ps - 1);

	m_cur = off;

	if (base == m_bloff) return;

	if (munmap(m_mmap, m_blsize) == -1) {
		ec = mk_ec(errno);
		return;
	}

	m_mmap = m_data = nullptr;

	size_t canMap = m_filesize - base;
	size_t willMap = canMap > m_mmapblock ? m_mmapblock : canMap;

	m_bloff = base;
	m_blsize = willMap;

	if (willMap != 0) {
		if ((m_mmap = m_data = (char*)mmap(NULL, willMap, get_protfl(m_flags), MAP_SHARED, m_fd, m_bloff)) == (void*)-1) {
			ec=mk_ec(errno);
			m_blsize=0;
			return;
		}
	}
}

fs_t source_mmap::size()
{
	return m_filesize;
}

source_istream::source_istream(std::istream& s, size_t cache_size)
:	m_cache_size(cache_size)
,	m_stream(s)
{
	zero();

	auto p=m_stream.tellg();
	m_stream.seekg(0, std::ios_base::end);
	m_file_size=m_stream.tellg();
	m_stream.seekg(p, std::ios_base::beg);

	if (!m_cache_size) m_cache_size = m_file_size;

	m_data=new char[m_cache_size];
}

source_istream::~source_istream()
{
	delete[] m_data;
}

void source_istream::prefetch()
{
	if (m_cur >= m_blsize) {
		m_cur = 0;
		m_bloff += m_blsize;
		m_blsize = m_stream.read(m_data, m_cache_size).gcount();
	}
}

void source_istream::make_available(std::error_code& ec)
{
	fs_t foff = m_bloff + m_cur;
	m_stream.seekg(foff, std::ios_base::beg);
	m_blsize = m_stream.read(m_data, m_cache_size).gcount();
	m_bloff = foff;
	m_cur = 0;
}

fs_t source_istream::size()
{
	return m_file_size;
}


void source_buffer::make_available(std::error_code& ec)
{
	return;
}

fs_t source_buffer::size()
{
	return m_blsize;
}

__attribute__((noinline)) bool do_test1(source& s, size_t itsize) 
{
	std::error_code ec;
	std::unique_ptr<char[]> buf(new char[itsize]);
	for (size_t x=0; x<itsize*2; ) {
		size_t got=s.fetch(buf.get(), itsize, ec);
		x+=got;
		printf("got: '%.*s'\n", (int)got, buf.get()); 
		if (got==0) break;
	}
	return true;
}


bool do_test()
{
	std::string s("hello hello world");
	std::stringstream ss(s);

	source_istream si(ss);

	do_test1(si, 6);

	printf("avail:%zu size:%llu\n", si.avail(), si.size()); 
	si.prefetch();
	printf("avail:%zu size:%llu\n", si.avail(), si.size()); 
	
	std::error_code ec;
	char buf[32];
	while (si.avail() != 0) {
		size_t got=si.fetch(buf, 4, ec);
		if (ec) {
			printf("error %d, %s\n", ec.value(), ec.message().c_str());
			break;
		}
		printf("got %zu '%.*s', avail:%zu\n", got, (int)got, buf, si.avail());
	}

	return true;
}
	
#ifdef _DEBUG
void checkerrno() {
	if (errno != 0)
		printf("errno: %d\n", errno);
std::error_code}
#else
	inline void checkerrno() {}
#endif

const char basic_header::pulmotor_magic_text[basic_header::magic_chars] = { 'p', 'u', 'l', 'M' };
static char const* error_desc [(unsigned)error::count] =
{
		"ok", "eof", "read-fail", "write-fail", "out-of-space"
};

char const* error_to_text( error code )
{
	return error_desc [(unsigned)code];
}
	
basic_input_buffer::~basic_input_buffer()
{}

basic_output_buffer::~basic_output_buffer()
{}

//
cfile_output_buffer::cfile_output_buffer (pulmotor::path_char const* file_name, std::error_code& ec)
#ifdef _DEBUG
	:	name_ (file_name)
#endif
{
	ec.clear();
	
#ifdef _WIN32
	file_ = _wfopen (file_name, L"wb+");
#else
	file_ = fopen (file_name, "wb+");
#endif
	
	if (!file_)
		ec.assign((int)std::errc::no_such_file_or_directory, std::system_category());
}

cfile_output_buffer::~cfile_output_buffer ()
{
	if (file_)
		fclose (file_);
}

size_t cfile_output_buffer::write (void const* src, size_t count, std::error_code& ec)
{
	if (!file_) {
		ec.assign((int)std::errc::bad_file_descriptor, std::system_category());
		return 0;
	}
	
	size_t res = fwrite (src, 1, count, file_);
	if (res == count)
	{
		ec.clear();
		return res;
	}
	
	if(ferror(file_))
		ec.assign((int)std::errc::io_error, std::system_category());
	
	return res;
}

cfile_input_buffer::cfile_input_buffer (pulmotor::path_char const* file_name, std::error_code& ec)
#ifdef _DEBUG
	:	name_ (file_name)
#endif
{
	ec.clear();
	
#ifdef _WIN32
	file_ = _wfopen (file_name, L"rb");
#else
	file_ = fopen (file_name, "rb");
#endif
	
	if (!file_)
		ec.assign((int)std::errc::no_such_file_or_directory, std::system_category());
}

cfile_input_buffer::~cfile_input_buffer ()
{
	if (file_)
		fclose (file_);
}

size_t cfile_input_buffer::read (void* dest, size_t count, std::error_code& ec)
{
	if (!file_) {
		ec.assign((int)std::errc::bad_file_descriptor, std::system_category());
		return 0;
	}
	
	size_t res = fread (dest, 1, count, file_);
	if (res == count) {
		ec.clear();
		return res;
	}
	
	if (feof (file_) || ferror(file_)) {
		ec.assign((int)std::errc::io_error, std::system_category());
	}
	
	return res;
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


std::unique_ptr<basic_input_buffer> create_plain_input (pulmotor::path_char const* file_name)
{
	std::unique_ptr<basic_input_buffer> ptr;
	
	std::error_code ec;
	cfile_input_buffer* p = new cfile_input_buffer (file_name, ec);
	if (!ec && p->good())
		ptr.reset(p);

	return std::move(ptr);
}

std::unique_ptr<basic_output_buffer> create_plain_output (pulmotor::path_char const* file_name)
{
	std::unique_ptr<basic_output_buffer> ptr;

	std::error_code ec;
	cfile_output_buffer* p = new cfile_output_buffer(file_name, ec);
	if (!ec && p->good())
		ptr.reset(p);
	
	return std::move(ptr);
}

fs_t file_size (pulmotor::path_char const* file_name, std::error_code& ec)
{
	struct stat s;
	if (stat (file_name, &s) == 0)
		return s.st_size;
	
	ec = mk_ec(errno);
	return 0;
}
	
}
