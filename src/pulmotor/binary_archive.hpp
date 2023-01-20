#ifndef PULMOTOR_BINARY_ARCHIVE_HPP
#define PULMOTOR_BINARY_ARCHIVE_HPP

#include "pulmotor_config.hpp"
#include "pulmotor_types.hpp"

#include <vector>
#include <fstream>
#include <unordered_map>

#include "stream.hpp"
#include "archive.hpp"


namespace pulmotor
{

template<class Derived>
struct archive_write_util
{
    enum { forced_align = 256 };
    enum { is_stream_aligned = true };

    archive_write_util(unsigned flags) : m_flags(flags) {}

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

    template<size_t Align, class T>
    void write_basic_aligned(T const& o) {
        constexpr size_t a = Align == 0 ? alignof(T) : Align;
        if constexpr (a == 4)
            write_default_aligned(&o, sizeof o);
        else
            write_aligned(&o, sizeof o, a);
    }

    void write_data_aligned(void const* p, size_t size, unsigned align)  {
        assert(align != 0);
        write_aligned(p, size, align);
    }

    // TODO: assert obj is a "proper" type, for example, not an array
    template<class T>
    void write_object_prefix(T const* obj, prefix pre);

private:
    void write_object_prefix_impl(char const* obj_name, size_t obj_name_len, prefix pre);
    void write_aligned(void const* p, size_t size, unsigned align);
    void write_default_aligned(void const* p, size_t size);

private:
    unsigned m_flags;
};

template<class Derived>
PULMOTOR_NOINLINE
void archive_write_util<Derived>::write_aligned(void const* p, size_t size, unsigned align) {
    self().align_stream(align);
    self().write_data(p, size);
}

template<class Derived>
PULMOTOR_NOINLINE
void archive_write_util<Derived>::write_default_aligned(void const* p, size_t size) {
    self().align_stream(4);
    self().write_data(p, size);
}

template<class Derived>
template<class T>
PULMOTOR_NOINLINE
void archive_write_util<Derived>::write_object_prefix(T const* obj, prefix pre) {
    assert(pre.version != vfl::no_version);

    if (obj == nullptr)
        pre.version |= (unsigned)vfl::null_ptr;

    write_object_prefix_impl(typeid(T).name(), strlen(typeid(T).name()), pre);
}

template<class Derived>
PULMOTOR_NOINLINE
void archive_write_util<Derived>::write_object_prefix_impl(char const* obj_name, size_t obj_name_len, prefix pre)
{
    fs_t block_size = 0;
    if ((m_flags & vfl::align_object)) {// || (m_flags & ver_flag_debug_string)) {
        pre.flags |= vfl::garbage;
    }

    u16 name_length = 0;
    if (m_flags & vfl::debug_string) {
        pre.flags |= vfl::debug_string;

        name_length = obj_name_len;
        if (name_length > (u16)vfl::debug_string_max_size) name_length = (u16)vfl::debug_string_max_size;
        block_size += sizeof name_length + name_length;
    }

    u16 garbage_len = block_size;
    if (m_flags & vfl::align_object) {
        pre.flags |= vfl::align_object;
        block_size += sizeof garbage_len;
    }

    // write version, then calculate how much we'll actually write afterwards
    self().template write_single<0>(pre.version);
    self().template write_single<0>(pre.flags);

    // [version] [garbage-len]? ([string-length] [string-data)? [ ... alignment-data ... ]? [object]
    fs_t base = self().offset();

    fs_t objs = 0;
    if(pre.is_aligned()) {
        objs = util::align(base + block_size, forced_align);
        garbage_len = objs - base - sizeof garbage_len;
    }

    if (pre.has_garbage()) {
        self().write_basic(garbage_len);
    }

    if (pre.has_dbgstr()) {
        self().write_basic(name_length);
        self().write_data(obj_name, name_length);
    }

    if (pre.is_aligned()) {
        assert(objs >= self().offset());
        size_t align_size = objs - self().offset();
#if PULMOTOR_DEBUG_ARCHIVE
        for (size_t i=align_size; i --> 0; )
            self().write_basic((u8)(i>0 ? '-' : '>'));
#else
        for (size_t i=align_size; i>0; ) {
            size_t count = i > 32 ? 32 : i;
            self().write_data(null_32, count);
            i -= count;
        }
#endif
        assert(self().offset() == util::align(self().offset(), forced_align) && "garbage alignment is not correct");
    }
}

template<class Derived>
struct archive_vu_util
{
    template<class Tstore, class Tq>
    void write_vu(Tq const& q)
    {
        Tstore u[util::euleb_count<Tstore, Tq>::value];
        size_t c = util::euleb(q, u);
        static_cast<Derived*>(this)->write_data(u, c);
    }

    template<class Tstore, class Tq>
    void read_vu(Tq& q)
    {
        size_t u = 0;
        int state=0;
        Tstore v;
        do static_cast<Derived*>(this)->read_single(v); while (util::duleb(u, state, v));
        q = u;
    }
};

template<class Derived>
struct archive_read_util
{
    enum { is_stream_aligned = true };
    Derived& self() { return *static_cast<Derived*>(this); }

    template<size_t Align, class T>
    void read_basic_aligned(T& o) {
        constexpr size_t a = Align == 0 ? alignof(T) : Align;
        if constexpr (a == 4)
            read_default_aligned(o);
        else
            read_aligned(o, a);
    }

    void align_stream(size_t al) {
        fs_t offset = self().offset();
        if ((offset & (al-1)) != 0)
            self().advance(util::align(offset, al) - offset);
    }

    prefix process_prefix();

private:
    template<class T> void read_default_aligned(T& o);
    template<class T> void read_aligned(T& o, size_t align);
};

template<class Derived>
template<class T>
PULMOTOR_NOINLINE
void archive_read_util<Derived>::read_default_aligned(T& o) {
    self().align_stream(4);
    self().read_basic(o);
}

template<class Derived>
template<class T>
PULMOTOR_NOINLINE
void archive_read_util<Derived>::read_aligned(T& o, size_t align) {
    self().align_stream(align);
    self().read_basic(o);
}

template<class Derived>
PULMOTOR_NOINLINE
prefix archive_read_util<Derived>::process_prefix() {

    prefix pre;

    u16 garbage, debug_string_len=0;
    self().template read_single<0>(pre.version);
    self().template read_single<0>(pre.flags);

    if (pre.has_garbage()) {
        self().read_single(garbage);
        self().advance(garbage);
    } else {
        if (pre.has_dbgstr()) {
            self().read_single(debug_string_len);
            self().advance(debug_string_len);
        }
    }

    return pre;
}

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

template<class D>
struct binary_archive_impl
    : archive
    , archive_read_util<binary_archive_impl<D>>
    , archive_vu_util<binary_archive_impl<D>>
{
    binary_archive_impl (source& s) : source_ (s) {}
    fs_t offset() const { return source_.offset(); }

    enum { is_reading = 1, is_writing = 0 };

    void advance(size_t s) {
        assert(source_.offset() + s <= source_.avail());
        source_.advance(s, ec_);
    }

    template<class T>
    void read_single(T& data) {
        assert( sizeof(T) <= source_.avail());
        assert( ((uintptr_t)source_.data() & (sizeof(T)-1)) == 0 && "stream alignment issues");
        data = *reinterpret_cast<T*>(source_.data());
        source_.advance(sizeof(T), ec_);
    }

    template<class T>
    void read_data (T* dest, size_t size) {
        assert( size <= source_.avail());
        memcpy( dest, source_.data(), size * sizeof(T));
        source_.advance( size, ec_ );
    }

    std::error_code ec_;

private:
    source& source_;

};

struct archive_whole
    : archive
    , archive_read_util<archive_whole>
    , archive_vu_util<archive_whole>
{
    archive_whole (source& s) : source_ (s) {}
    fs_t offset() const { return source_.offset(); }

    enum { is_reading = 1, is_writing = 0 };

    void advance(size_t s) {
        assert(source_.offset() + s <= source_.avail());
        source_.advance(s, ec_);
    }

    template<class T>
    void read_single(T& data) {
        assert( sizeof(T) <= source_.avail());
        assert( ((uintptr_t)source_.data() & (sizeof(T)-1)) == 0 && "stream alignment issues");
        data = *reinterpret_cast<T*>(source_.data());
        source_.advance(sizeof(T), ec_);
    }

    template<class T>
    void read_data (T* dest, size_t size) {
        assert( size <= source_.avail());
        memcpy( dest, source_.data(), size);
        source_.advance( size, ec_ );
    }

    std::error_code ec_;

private:
    source& source_;
};

struct archive_chunked
    : archive
    , archive_read_util<archive_chunked>
    , archive_vu_util<archive_chunked>
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

struct archive_istream
    : archive
    , archive_read_util<archive_istream>
    , archive_vu_util<archive_istream>
{
    struct context { };

    archive_istream (std::istream& s) : ec_{}, stream_ (s){}

    fs_t offset() const { return stream_.tellg(); }

    enum { is_reading = 1, is_writing = 0 };

    void advance(size_t s) {
        stream_.seekg(stream_.tellg() + std::streamoff(s));
    }

    template<class T>
    void read_basic(T& data) {
        stream_.read((char*)&data, sizeof data);
    }

    void read_data (void* dest, size_t size) {
        stream_.read((char*)dest, size);
    }

    std::error_code ec_;

private:
    std::istream& stream_;
};

inline constexpr u64 pack_sa(size_t s, size_t a) { return (s << 8) | (a & 0xff); }

struct archive_sink
    : public archive
    , public archive_write_util<archive_sink>
    , public archive_pointer_support<archive_sink>
    , public archive_vu_util<archive_sink>
{
    sink& sink_;
    fs_t written_;
    vfl m_flags;

public:
    archive_sink (sink& s, unsigned flags = 0)
        : archive_write_util<archive_sink>(flags)
        , sink_ (s)
        , written_ (0)
    {}

    enum { is_reading = 0, is_writing = 1 };

    fs_t offset() const { return written_; }

    template<class T>
    void write_single (T const& data) {
        write_with_error_check_p(&data, pack_sa(sizeof data, type_align<T>::value));
    }

    template<class T>
    void write_data (T const* src, size_t size) {
        write_with_error_check(src, sizeof(T) * size, type_align<T>::value);
    }

    void write_prefix(prefix pre, prefix_ext const* ppx = nullptr);

private:
    void align_stream(size_t alignment);
    void write_with_error_check(void const* src, size_t size, size_t alignment);
    void write_with_error_check_p(void const* src, u64 size_align);
};

struct archive_memory
    : archive
    , archive_vu_util<archive_memory>
{
    // source& source_;
    // size_t m_offset;

    source& source_;

    archive_memory (source& src) : source_ (src) {}

    fs_t offset() const { return source_.offset(); }

    enum { is_reading = 1, is_writing = 0 };

    template<class T>
    void read_single(T& data) {
#if 1
        // read_aligned_with_error_check(&data, sizeof(T), type_align<T>::value);
        read_aligned_with_error_check_p(&data, pack_sa(sizeof(T), type_align<T>::value));
#else
        assert( sizeof(T) <= source_.avail());
        assert( util::is_aligned(m_offset, alignof(T)) && "stream alignment issues");

        // data = *reinterpret_cast<T*>(source_.data() + m_offset);
        m_offset = util::align(m_offset, type_align<T>::value);
        memcpy(&data, source_.data() + m_offset, sizeof data);
        m_offset += sizeof data;
        //source_.advance(sizeof(T), ec_);
#endif
    }

    template<class T>
    void read_data (T* dest, size_t size) {
        assert( size <= source_.avail());
        assert( util::is_aligned(source_.offset(), alignof(T)) && "stream alignment issues");
        memcpy (dest, source_.data() + source_.offset(), size * sizeof(T));
        std::error_code ec;
        source_.advance( size, ec );
    }

    prefix read_prefix(prefix_ext* ext);

private:
    void read_aligned_with_error_check(void* dest, size_t size, size_t alignment);
    void read_aligned_with_error_check_p(void* dest, u64 size_align);
};

struct archive_vector_out
    : public archive
    , public archive_write_util<archive_vector_out>
    , public archive_pointer_support<archive_vector_out>
    , public archive_vu_util<archive_vector_out>
{
    std::vector<char> data;

    archive_vector_out(unsigned version_flags = 0)
        : archive_write_util<archive_vector_out>(version_flags)
    {}

    enum { is_reading = false, is_writing = true };

    size_t offset() const { return data.size(); }

    template<class T>
    void write_single(T const& a) {
        char const* p = (char const*)&a;
        data.insert(data.end(), p, p + sizeof(a));
    }

    void write_data(void const* src, size_t size) {
        data.insert(data.end(), (char const*)src, (char const*)src + size);
    }

    std::string str() const { return std::string(data.data(), data.size()); }
};

struct archive_vector_in
    : public archive
    , public archive_read_util<archive_vector_in>
    , public archive_pointer_support<archive_vector_in>
    , public archive_vu_util<archive_vector_out>
{
    std::vector<char> data;
    size_t m_offset = 0;

    archive_vector_in(std::vector<char> const& i) : data(i) {}

    enum { is_reading = true, is_writing = false };

    size_t offset() const { return m_offset; }

    template<class T>
    void read_single(T& a) {
        a = *(T const*)(data.data() + m_offset);
        m_offset += sizeof a;
    }

    void advance(size_t s) { m_offset += s; }
    void read_data(void* src, size_t size) {
        memcpy(src, data.data() + m_offset, size); m_offset += size;
    }

    std::string str() const { return std::string(data.data(), data.size()); }
};

// template<class T>
// size_t load_archive (path_char const* pathname, T&& obj, std::error_code& ec)
// {
// 	source_mmap s (pathname, source_mmap::ro, ec);
// 	if (ec)
// 		return 0U;

// 	archive_whole ar(s);
// 	ar | std::forward<T>(obj);
// 	ec = ar.ec_;

// 	return ar.offset();
// }

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

// template<class T>
// size_t save_archive (path_char const* pathname, T&& obj, std::error_code& ec)
// {
// 	std::fstream os(pathname, std::ios_base::binary|std::ios_base::out);
// 	if (os.good()) {
// 		ec = std::make_error_code(std::errc::invalid_argument);
// 		return 0;
// 	}

// 	sink_ostream s(os);
// 	archive_sink ar(s);

// 	ar | std::forward<T>(obj);

// 	return ar.offset();
// }

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


#endif // PULMOTOR_BINARY_ARCHIVE_HPP
