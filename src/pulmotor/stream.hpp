#ifndef PULMOTOR_CONTAINER_HPP_
#define PULMOTOR_CONTAINER_HPP_

#include <memory>
#include <iterator>
#include <sstream>

#include "pulmotor_config.hpp"
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

class block
{
    block(block const& a) = delete;
    block& operator=(block const& a) = delete;

public:
    block() { zero_block(); }

    // absolute offset of 'consumed' pointer
    fs_t offset() const { return m_bloff + m_cur; }

    // number of bytes available for immediate consumption (starting at current data)
    size_t avail() { return m_blsize - m_cur; }

    // ptr to current pointer
    char* data() { return m_data + m_cur; }

protected:
    char* m_data;
    fs_t m_bloff; // block offset in stream
    size_t m_blsize, m_cur; // block size and offset in block

    void zero_block() { m_data=nullptr; m_bloff=0; m_blsize=0; m_cur=0; }
};

class source : public block
{
public:
    virtual ~source() = 0;
    virtual fs_t size() = 0;

    void advance(size_t sz, std::error_code& ec);
    size_t fetch(void* dest, size_t sz, std::error_code& ec);

protected:
    virtual void make_available(std::error_code& ec) = 0;
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

    void reset();

    virtual void make_available(std::error_code& ec);

public:
    source_mmap();
    source_mmap(path_char const* path, flags flags, std::error_code& ec, fs_t offset = 0, size_t mmap_block_size = 0)
    : source_mmap()
    {
        map(path, flags, ec, offset, mmap_block_size);
    }
    ~source_mmap();

    void map(path_char const* path, flags flags, std::error_code& ec, fs_t offset = 0, size_t mmap_block_size = 0);
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

class source_buffer : public source
{
public:
    source_buffer(char const* data, size_t data_size)
    {
        m_data = const_cast<char*>(data);
        m_bloff = 0;
        m_blsize = data_size;
        m_cur = 0;
    }

    fs_t size() override;
    void make_available(std::error_code& ec) override;
};

class sink
{
public:
    virtual ~sink() = 0;
    virtual void write(void const* data, size_t size, std::error_code& ec) = 0;
};

class sink_ostream : public sink
{
    std::ostream& m_stream;

public:
    sink_ostream(std::ostream& os);
    sink_ostream(sink_ostream const& os) = default;
    ~sink_ostream();

    void write(void const* data, size_t size, std::error_code& ec);
};

class sink_sstream : public sink
{
    std::stringstream m_stream;

public:
    sink_sstream();
    sink_sstream(sink_sstream const& s);
    ~sink_sstream();

    void write(void const* data, size_t size, std::error_code& ec);
    std::stringstream& ss() { return m_stream; }
    std::stringstream const& ss() const { return m_stream; }
};

class sink_vector : public sink
{
    std::vector<char> m_data;

public:
    sink_vector();
    sink_vector(sink_vector const& s);
    ~sink_vector();

    void write(void const* data, size_t size, std::error_code& ec);
    std::vector<char> const& data() { return m_data; }
};

struct PULMOTOR_ATTR_DLL header
{
    static const u32 magic_str = 'Mlup'; // little endian

public:
    enum class flags : u32
    {
        plain	= 0x0000,
        zlib	= 0x0001,
        bzip2	= 0x0002,
        lzma	= 0x0003,
        be		= 0x0100,
        checksum= 0x0200,
    };

    u32				magic;
    unsigned short	version;
    unsigned short	flags;
};

// global input/output factory functions
fs_t file_size (pulmotor::path_char const* file_name, std::error_code& ec);
fs_t file_size (pulmotor::path_char const* file_name);

} // pulmotor

#endif // PULMOTOR_CONTAINER_HPP_
