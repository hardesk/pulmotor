#include "binary_archive.hpp"
#include <concepts>

namespace pulmotor {


size_t fill_sink(sink& s, size_t size, std::error_code& ec) {
    size_t written = 0;
    while(1) {
        size_t write = std::min(size, (size_t)32u);
        if (write) {
            s.write(null_32, write, ec);
            if (ec) return written;
            size -= write;
            written += write;
        } else
            break;
    }
    return written;
}

template<align_policy AlignPolicy>
size_t write_with_error_check_impl(sink& sink_, std::error_code& ec, fs_t& offset, void const* src, size_t size, size_t naturalAlign, AlignPolicy const& ap) {

    size_t written = 0;
    fs_t aligned = ap.align(offset, naturalAlign);
    if (aligned != offset) {
        size_t fill = aligned - offset;
        written += fill_sink(sink_, fill, ec);
        if (ec) return written;
    }

    sink_.write (src, size, ec);
    written += size;
    return written;
}

PULMOTOR_NOINLINE void archive_sink::write_with_error_check(void const* src, size_t size, size_t alignment) {
    std::error_code ec;
    written_ += write_with_error_check_impl(sink_, ec, written_, src, size, alignment, *this);
    if (ec) PULMOTOR_THROW_SYSTEM_ERROR(ec, "error while writing data");
}

void archive_sink::write_with_error_check_p(void const* src, u64 sa) {
    write_with_error_check(src, sa >> 8, sa & 0xff);
}

#if 0
template<class AlignPolicy>
void binary_write_prefix(sink& sink_, std::error_code& ec, fs_t& offset, vfl flags, prefix pre, prefix_ext const* ppx, AlignPolicy ap) {

    size_t forced_align = 32;
    // <version> <flags> [<garbage-len> [<name-len> <name>] [<align-data>] ]
    fs_t block_size = sizeof(pre);
    u16 garbage_len = 0, align_fill_size = 0, name_length = 0;

    ap.align(sink_, sizeof pre.version);
    sink_.write(&pre.version, sizeof pre.version, ec);
    offset += sizeof pre.version;
    if (ec) goto error_writing;

    // We want to be able to skip all garbage between version-flag and the object data in one go
    // when reading. Thus if anything extra we mark that we have garbage and include that flag.

    if (ppx) pre.flags |= vfl::debug_string; else pre.flags &= vfl::debug_string;

    if (((pre.flags & flags) & vfl::debug_string) != 0) {
        pre.flags |= vfl::garbage;
        block_size += sizeof garbage_len;

        name_length = ppx->class_name.size();
        if (name_length > (u16)vfl::debug_string_max_size) name_length = (u16)vfl::debug_string_max_size;
        block_size += sizeof name_length + name_length;
    }

    if ((flags & vfl::align_object) != 0) {
        fs_t obj_start = util::align(offset + block_size, forced_align);
        garbage_len = obj_start - offset - sizeof garbage_len;
        align_fill_size = obj_start - (offset + block_size);
    }

    // block_size += sizeof garbage_len + garbage_len;
    assert((flags & vfl::align_object) == 0 || ((offset + garbage_len + garbage_len) % forced_align) == 0);

    sink_.write(&pre.flags, sizeof pre.flags, ec);
    if (ec) goto error_writing;

    if (pre.has_garbage()) {
        sink_.write(&garbage_len, sizeof garbage_len, ec);
        offset += sizeof garbage_len;
        if (ec) goto error_writing;

        if (pre.has_dbgstr() && ppx) {
            sink_.write(&name_length, sizeof name_length, ec);
            offset += sizeof name_length;
            if (ec) goto error_writing;
            sink_.write(ppx->class_name.c_str(), name_length, ec);
            offset += name_length;
            if (ec) goto error_writing;
        }

        while(align_fill_size > 0) {
            fs_t off = offset;
            size_t write = std::min(align_fill_size, (u16)32U);
            sink_.write(null_32, write, ec);
            offset += write;
            if (ec) goto error_writing;
            align_fill_size -= write;
        }
    }

    return ec;

error_writing:
    return ec;
}
#else

// <u16-version> <u16-flags> [<u16-garbage-len> [<vu-name-len> <chararr-name>] [<arr-align-data>] ]
template<class AlignPolicy>
size_t binary_write_prefix(sink& sink_, std::error_code& ec, fs_t offset, vfl flags, prefix pre, prefix_ext const* ppx, AlignPolicy ap) {

    size_t align_ext = ap.align(1u, type_align<u16>::value) * 4; // 4: three for version+flags+garbage-len and one for vu-name-len
    size_t buff_size = (size_t)vfl::debug_string_max_size + forced_align_value * 2 + align_ext;
    char* buffer = (char*)alloca(buff_size);

    auto store = [&ap, buffer](size_t off, size_t align, void const* data, size_t size) -> size_t
    {
        size_t aligned = ap.align(off, align);
        if (aligned != off)
            memset(buffer + off, 0, aligned - off);
        memcpy(buffer + aligned, data, size);
        return aligned + size;
   };

   auto align_only = [&ap, buffer](size_t off, size_t align) -> size_t {
        size_t aligned = ap.align(off, align);
        if (aligned != off)
            memset(buffer + off, 0, aligned - off);
        return aligned;
   };

    // We want to start "building" data with an offset that matches the global offset, otherwise alignments may be calculated wrong.
    size_t boffset = offset & (forced_align_value-1);
    size_t buff_start = boffset;

    // We want to be able to skip all garbage between version-flag and the object data in one go
    // when reading. Thus if anything extra we mark that we have garbage and include that flag.

    if (ppx) pre.flags |= vfl::debug_string; else pre.flags &= vfl::debug_string;
    if ((flags & vfl::align_object) != 0) pre.flags |= vfl::align_object;
    if ((pre.flags & (vfl::align_object|vfl::debug_string)) != 0) pre.flags |= vfl::garbage;

    u16 name_length = 0;
    if ((pre.flags & vfl::debug_string) != 0) {
        assert(ppx);
        name_length = ppx->class_name.size();
        if (name_length > (u16)vfl::debug_string_max_size)
            name_length = (u16)vfl::debug_string_max_size;
    }

    boffset = store(boffset, type_align<prefix>::value, &pre, sizeof pre);

    if (pre.has_garbage()) {
        size_t garbage_offset = boffset;
        u16 garbage_len = 0;
        boffset = store(boffset, type_align<u16>::value, &garbage_len, sizeof(u16));

        if (pre.has_dbgstr()) {
            assert(ppx);
            boffset = align_only(boffset, type_align<prefix_vu_t>::value);
            boffset += util::euleb(name_length, (prefix_vu_t*)(buffer + boffset)) * sizeof(prefix_vu_t);
            boffset = store(boffset, 1, ppx->class_name.c_str(), name_length);
        }

        if (pre.is_aligned())
            boffset = align_only(boffset, forced_align_value);

        garbage_len = boffset - garbage_offset - sizeof(garbage_len);
        store(garbage_offset, type_align<u16>::value, &garbage_len, sizeof garbage_len);
    }

    assert(boffset <= buff_size && "buffer overrun");

    size_t write = boffset - buff_start;
    sink_.write(buffer + buff_start, write, ec);
    return write;
}
#endif

template<align_policy AlignPolicy>
size_t binary_read_prefix_impl(source& src, std::error_code& ec, prefix& pre, prefix_ext* ppx, AlignPolicy ap) {
    size_t was_read = 0, max_forced_align = forced_align_value;

    // <version> <flags> [<garbage-len> [<name-len> <name>] [<align-data>] ]
    was_read += src.fetch(&pre, sizeof pre, ec);
    if (ec) return was_read;
    if (pre.has_garbage()) {
        u16 garbage_len = 0;
        was_read += src.fetch(&garbage_len, sizeof garbage_len, ec);
        if (ec) return was_read;
        char* garbage = (char*)alloca((size_t)vfl::debug_string_max_size + max_forced_align * 2);
        if (ppx) {
            was_read += src.fetch(garbage, garbage_len, ec);
            if (ec) return was_read;

            u16& name_len = *(u16*)&garbage[0];
            char const* name_data = garbage + 2;
            ppx->class_name = std::string_view(name_data, name_len);

        } else if (garbage_len != 0) {
            src.advance(garbage_len, ec);
            was_read += garbage_len;
            if (ec) return was_read;
        }
    }

    return was_read;
}

void archive_sink::align_stream(size_t al) {
    while(1) {
        fs_t off = offset();
        size_t write = util::align(off, al) - off;
        if (write)
            write_data(null_32, write < 32 ? write : 32);
        else
            break;
    }
}

void archive_sink::begin_object(prefix const& px, prefix_ext* pxx) {
    if (px.store()) {
        std::error_code ec;
        written_ += binary_write_prefix(sink_, ec, written_, m_flags, const_cast<prefix&>(px), pxx, data_align_policy{});
        if (ec) PULMOTOR_THROW_SYSTEM_ERROR(ec, "while writing prefix");
    }
}

void archive_sink::write_size(size_t size) {
    std::error_code ec;
    vu_size_t temp[util::euleb_max<vu_size_t, size_t>::value];
    size_t count = util::euleb(size, temp);
    written_ += write_with_error_check_impl(sink_, ec, written_, temp, count * sizeof(vu_size_t), type_align<vu_size_t>::value, *this);
    if (ec) PULMOTOR_THROW_SYSTEM_ERROR(ec, "error while writing vu size");
}

template<align_policy AlighPolicy>
void read_aligned_with_error_check_impl(source& src, std::error_code& ec, void* dest, size_t size, size_t alignment, AlighPolicy ap)
{
    fs_t offset = src.offset();
    fs_t expected = ap.align(offset, alignment);
    size_t fwd = expected - offset;
    if (fwd) {
        src.advance(fwd, ec);
        if (ec) return;
    }

    // if (src.avail() < size) {
    //     ec = std::make_error_code(std::errc::io_error);
    //     return;
    // }

    // memcpy(dest, src.data() + expected, size);
    src.fetch(dest, size, ec);
}

// prefix archive_source::read_prefix(prefix_ext* ext)
// {
//     std::error_code ec;
//     prefix pre;
//     binary_read_prefix_impl(source_, ec, pre, ext, data_align_policy{});
//     return pre;
// }

// size_t archive_source::read_size()
// {
//     size_t state = 0;
//     return 0;
// }

// void archive_source::read_aligned_with_error_check(void* dest, size_t size, size_t alignment)
// {
//     std::error_code ec;
//     if (source_.avail() < size) {
//         source_.fetch(0, 0, ec);
//         if (ec) PULMOTOR_THROW_SYSTEM_ERROR(ec, "failed to fetch data");
//     }
//     // assert(sizeof(T) <= source_.avail());
//     // assert(util::is_aligned(m_offset, alignof(T)) && "stream alignment issues");

//     read_aligned_with_error_check_impl(source_, ec, dest, size, alignment, data_align_policy{});
//     if (ec) PULMOTOR_THROW_SYSTEM_ERROR(ec, "failed to read data");

//     // // data = *reinterpret_cast<T*>(source_.data() + m_offset);
//     // m_offset = util::align(m_offset, type_align<T>::value);
//     // memcpy(&data, source_.data() + m_offset, sizeof data);
//     // m_offset += sizeof data;
//     // //source_.advance(sizeof(T), ec_);
// }

// void archive_source::read_aligned_with_error_check_p(void* dest, u64 sa)
// {
//     read_aligned_with_error_check(dest, sa >> 8, sa & 0xff);
// }

void archive_memory::read_single(void* data, size_t size)
{
    memcpy(data, m_data, size);
    m_data += size;
}

void archive_memory::begin_object(prefix const& px, prefix_ext* pxx)
{
    // px comes with the values "from code", but is overwritten if a prefix is
    // expected to be present in the stream.
    if (px.store()) {
        prefix const* p = reinterpret_cast<prefix const*>(util::align(m_data, type_align<prefix>::value));
        m_data = (char const*)p + sizeof(prefix);

        if (p->has_garbage()) {
            u16 const* garbage_len = (u16 const*)util::align(m_data, type_align<u16>::value);
            if (p->has_dbgstr() && pxx) {
                size_t name_len = 0;
                char const* name_len_e = util::align((char const*)garbage_len + sizeof(u16), type_align<prefix_vu_t>::value);
                char const* name_start = (char const*)garbage_len + sizeof(u16) + util::decode(name_len, (prefix_vu_t*)name_len_e) * sizeof(prefix_vu_t);
                pxx->class_name = std::string_view(name_start, name_len);
            }
            m_data += *garbage_len;
        }
        const_cast<prefix&>(px) = *p;
    }
}

void archive_memory::assing_unaligned(u8* p) {
    *p = *(u8 const*)m_data;
    m_data += sizeof(u8);
}
void archive_memory::assing_unaligned(u16* p) {
    *p = *(u16 const*)m_data;
    m_data += sizeof(u16);
}
void archive_memory::assing_unaligned(u32* p) {
    *p = *(u32 const*)m_data;
    m_data += sizeof(u32);
}
void archive_memory::assing_unaligned(u64* p) {
    *p = *(u64 const*)m_data;
    m_data += sizeof(u64);
}

size_t archive_memory::read_size() {
    char const* p = (char const*)align((uintptr_t)m_data, type_align<vu_size_t>::value);

    size_t size = 0;
    size_t adv = util::decode(size, reinterpret_cast<vu_size_t const*>(p)) * sizeof(vu_size_t);
    m_data += adv;
    return size;
}


} // pulmotor


pulmotor::XXX xx;
