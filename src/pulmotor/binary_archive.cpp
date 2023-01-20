#include "binary_archive.hpp"
#include <concepts>

namespace pulmotor {


template<class T>
concept align_policy = requires(T policy)
{
    { policy.align(fs_t(0), size_t(0u)) } -> std::convertible_to<fs_t>;
};

struct packed_align_policy
{
    template<std::unsigned_integral T>
    constexpr T align(T offset, size_t naturalAlignment) { return offset; }
};

struct min_align_policy
{
    size_t min_alignment = 1;
    template<std::unsigned_integral T>
    constexpr T align(T offset, size_t naturalAlignment) {
        return util::align(offset, min_alignment);
    }
};

struct natural_align_policy
{
    template<std::unsigned_integral T>
    constexpr T align(T offset, size_t naturalAlignment) {
        return util::align(offset, naturalAlignment);
    }
};

struct natural_min_align_policy
{
    size_t min_alignment;
    template<std::unsigned_integral T>
    constexpr T align(T offset, size_t naturalAlignment) {
        size_t target = naturalAlignment > min_alignment ? naturalAlignment : min_alignment;
        return util::align(offset, target);
    }
};

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
size_t write_with_error_check_impl(sink& sink_, std::error_code& ec, fs_t& offset, void const* src, size_t size, size_t naturalAlign, AlignPolicy ap) {

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

PULMOTOR_NOINLINE void archive_sink::write_with_error_check(void const* src, size_t size, size_t align) {
    std::error_code ec;
    written_ += write_with_error_check_impl(sink_, ec, written_, src, size, align, natural_align_policy{});
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

// <version> <flags> [<garbage-len> [<name-len> <name>] [<align-data>] ]
template<class AlignPolicy>
size_t binary_write_prefix(sink& sink_, std::error_code& ec, fs_t& offset, vfl flags, prefix pre, prefix_ext const* ppx, AlignPolicy ap) {

    size_t forced_align = 32;
    fs_t block_size = sizeof(pre);
    u16 garbage_len = 0, align_fill_size = 0, name_length = 0;

    auto store = [&ap](char* buffer, size_t off, size_t align, void const* data, size_t size) -> size_t
    {
        size_t aligned = ap.align(off, align);
        if (aligned != off)
            memset(buffer + off, 0, aligned - off);
        memcpy(buffer + aligned, data, size);
        return aligned + size;
    };

    char* buffer = (char*)alloca((size_t)vfl::debug_string_max_size + forced_align * 2 + 4 * 4);
    size_t buff_start = offset & (forced_align-1);
    size_t boffset = buff_start;

    boffset = store(buffer, boffset, sizeof pre.version, &pre.version, sizeof pre.version);

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
        fs_t obj_start = util::align(boffset + block_size, forced_align);
        garbage_len = obj_start - boffset - sizeof garbage_len;
        align_fill_size = obj_start - (boffset + block_size);
    }

    // block_size += sizeof garbage_len + garbage_len;
    assert((flags & vfl::align_object) == 0 || ((boffset + garbage_len + garbage_len) % forced_align) == 0);

    boffset = store(buffer, boffset, sizeof pre.flags, &pre.flags, sizeof pre.flags);

    if (pre.has_garbage()) {
        boffset = store(buffer, boffset, sizeof garbage_len, &garbage_len, sizeof garbage_len);

        if (pre.has_dbgstr() && ppx) {
            boffset = store(buffer, boffset, sizeof name_length, &name_length, sizeof name_length);
            boffset = store(buffer, boffset, 1, ppx->class_name.c_str(), name_length);
        }

        if (align_fill_size > 0) {
            memset(buffer + boffset, 0, align_fill_size);
            boffset += align_fill_size;
        }
    }

    size_t write = boffset - buff_start;
    sink_.write(buffer + buff_start, boffset - buff_start, ec);
    return write;
}
#endif

size_t binary_read_prefix_impl(source& src, std::error_code& ec, prefix& pre, prefix_ext* ppx) {
    size_t was_read = 0, max_forced_align = 32;

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

void archive_sink::write_prefix(prefix pre, prefix_ext const* ppx) {
    std::error_code ec;
    written_ += binary_write_prefix(sink_, ec, written_, m_flags, pre, ppx, packed_align_policy{});
    if (ec) PULMOTOR_THROW_SYSTEM_ERROR(ec, "while writing prefix");
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

prefix archive_memory::read_prefix(prefix_ext* ext)
{
    std::error_code ec;
    prefix pre;
    binary_read_prefix_impl(source_, ec, pre, ext);
    return pre;
}

void archive_memory::read_aligned_with_error_check(void* dest, size_t size, size_t alignment)
{
    std::error_code ec;
    if (source_.avail() < size) {
        source_.fetch(0, 0, ec);
        if (ec) PULMOTOR_THROW_SYSTEM_ERROR(ec, "failed to fetch data");
    }
    // assert(sizeof(T) <= source_.avail());
    // assert(util::is_aligned(m_offset, alignof(T)) && "stream alignment issues");

    read_aligned_with_error_check_impl(source_, ec, dest, size, alignment, natural_align_policy{});
    if (ec) PULMOTOR_THROW_SYSTEM_ERROR(ec, "failed to read data");

    // // data = *reinterpret_cast<T*>(source_.data() + m_offset);
    // m_offset = util::align(m_offset, type_align<T>::value);
    // memcpy(&data, source_.data() + m_offset, sizeof data);
    // m_offset += sizeof data;
    // //source_.advance(sizeof(T), ec_);
}

void archive_memory::read_aligned_with_error_check_p(void* dest, u64 sa)
{
    read_aligned_with_error_check(dest, sa >> 8, sa & 0xff);
}


} // pulmotor
