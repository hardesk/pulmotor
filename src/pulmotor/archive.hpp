#ifndef PULMOTOR_ARCHIVE_HPP
#define PULMOTOR_ARCHIVE_HPP

#include "pulmotor_config.hpp"
#include "pulmotor_types.hpp"

#include <type_traits>

#include <cstdarg>
#include <cstddef>
#include <cassert>
#include <algorithm>
#include <typeinfo>
#include <string>
#include <ios>
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

enum archiver_flags
{
};

struct binary_stream_archive
{
    void write_data(void const* p, size_t size);
    void read_data(void* p, size_t size);
};


template<class T>
struct is_archive_if : std::enable_if<std::is_base_of<archive, T>::value, T> {};

template<class Ar, class = void> struct has_supported_mods : std::false_type {};
template<class Ar>				 struct has_supported_mods<Ar, std::void_t<typename Ar::supported_mods> > : std::true_type {};

template<class Ar, class = void> struct supports_kv : std::false_type {};
template<class Ar>				 struct supports_kv<Ar, std::void_t<typename Ar::supports_kv> > : std::true_type {};

extern char null_32[32];

template<class Derived> struct mod_op { using type = Derived; };

template<class T, class... Ops>
struct mod_list : std::tuple<Ops...> {
    using value_ttype = T;
    T const& value;
    mod_list(T const& v, std::tuple<Ops...> const& a) : std::tuple<Ops...>(a), value(v) {}
};

template<class T> struct is_mod_list : std::false_type { };
template<class T, class... Ops> struct is_mod_list<mod_list<T, Ops...>> : std::true_type { };

template<class T, class Op>
mod_list<T, Op> operator*(T const& v, mod_op<Op> const& op) {
    return mod_list<T, Op>(v, std::tuple<Op>(static_cast<Op const&>(op)));
}

template<class T, class Op, class... Ops>
mod_list<T, Ops..., Op> operator*(mod_list<T, Ops...> const& ops, mod_op<Op> const& op) {
    auto op_tup = std::make_tuple(static_cast<Op const&>(op));
    std::tuple<Ops...> const& ops_tup = static_cast<std::tuple<Ops...> const&>(ops);
    return mod_list<T, Ops..., Op>(ops.value, std::tuple_cat(ops_tup, op_tup));
}

template<class Ar, class Context, class T, class... Ops>
void apply_mod_list(Ar& ar, Context& ctx, mod_list<T, Ops...> const& ops) {
    if constexpr (has_supported_mods<Ar>::value)
        pulmotor::util::map( [&ar, &ctx](auto op) {
            using op_t = decltype(op);
            if constexpr( Ar::supported_mods::template has< typename op_t::type >::value )
                op(ar, ctx);
        }, ops);
}

struct mod_base64 : mod_op<mod_base64> {
    template<class Ar, class Context>
    void operator()(Ar& ar, Context& ctx) { ctx.flags |= 0; }
};

struct mod_wrap : mod_op<mod_wrap> {
    int width;
    template<class Ar, class Context>
    void operator()(Ar& ar, Context& ctx) { ctx.wrap = width; }
};

// template<class Derived>
// struct archive_write_named_util
// {
// 	layout_registry m_reg;
// 	layout* m_current;
// 	std::vector<layout*> m_layout_stack;

// 	void begin_object()
// 	}

// 	template<size_t Align, class T>
// 	void write_named_aligned(std::string_view name, T const& o) {
// 		if (m_current) {
// 			m_current.reg(name, o);
// 		}

// 		write_aligned<Align>(name, o);
// 	}
// };
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

    void write_data (void const* src, size_t size)
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
            print_name (("not tracking version for '" + name + "'").c_str(), (int)get_meta<T>::value, off);
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

    void write_data (void const* src, size_t size)
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

} // pulmotor

#endif
