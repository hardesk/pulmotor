#ifndef PULMOTOR_YAML_ARCHIVE_HPP
#define PULMOTOR_YAML_ARCHIVE_HPP

#include "pulmotor_config.hpp"
#include "pulmotor_types.hpp"

#include <type_traits>

#include <cstdarg>
#include <cstddef>
#include <cassert>
#include <vector>
#include <algorithm>
#include <typeinfo>
#include <string>
#include <ios>
#include <fstream>
#include <unordered_map>
#include <concepts>

#include "archive.hpp"
#include "stream.hpp"
#include "util.hpp"
#include "yaml_emitter.hpp"

#include <c4/std/string.hpp>
#include <ryml.hpp>

//#include "traits.hpp"

//#include <stir/filesystem.hpp>

#ifdef PULMOTOR_EXCEPTIONS
#define PULMOTOR_ERROR(msg, ...) pulmotor_throw(msg, __VA_ARGS__)
#else
#define PULMOTOR_ERROR(msg, ...) pulmotor_abort(msg, __VA_ARGS__)
#endif
namespace pulmotor {

namespace yaml {
    struct yaml_emitter;
    struct encoder;
    struct writer;
}


struct u8p_exp // 3b exp + 5b mantisa (positive only): 2^7*32
{
    static unsigned unpack(u8 x) {
        unsigned e = x >> 5, m = x&0x1f;
        return (1 << e) * 32;
    }

    static u8 pack(unsigned x) {
        unsigned ee = x >> 5;
        if (ee == 0)
            return x;

        unsigned l = (sizeof(x)*8-1) - std::countl_zero(ee);
        if (l > 7) l = 7;

        unsigned m = (x >> l) & 0x1f;
        return (l << 5) | m;
    }
};

struct yaml_document
{
    ryml::Tree y;
    std::optional<location_map> loc;
    char const* m_text_start = nullptr;
    path_char const* m_file_name;

    ryml::Callbacks m_callbacks { this, nullptr, nullptr, &ryml_error };

    static void ryml_error( const char* msg, size_t msg_len, ryml::Location location, void *user_data ) {
        static_cast<yaml_document*>(user_data)->ryml_error(msg, msg_len, location);
    }

    yaml_document(char const* name);
    ~yaml_document();

    full_location lookup_location( ryml::csubstr const& s );
    void ryml_error( const char* msg, size_t msg_len, ryml::Location location );
    void parse( ryml::substr s, path_char const* filename, bool index_lines = true );

    std::string to_string();
};

enum issue_type
{
    issue_fatal,
    issue_error,
    issue_warning,
    issue_info
};

struct diagnostics
{
    void report_issue(issue_type issue, full_location const& loc, std::string const& msg) { };
};

template<class Derived>
struct yaml_archive_vu_util
{
    template<class Tstore, class Tq>
    void write_vu(Tq& q)
    {
        static_cast<Derived*>(this)->write_basic(q);
    }

    template<class Tstore, class Tq>
    void read_vu(Tq& q)
    {
        static_cast<Derived*>(this)->read_basic(q);
    }
};

ryml::NodeData const* find_with_retry(ryml::Tree const& y, size_t& cur_id, ryml::csubstr const& prop_name);

template<class Derived>
struct yaml_archive_read_util
{
    enum { is_stream_aligned = false };

    Derived& self() { return *static_cast<Derived*>(this); }
    yaml_document& yaml() { return *static_cast<Derived*>(this)->y_doc; }
    auto& diag() { return static_cast<Derived*>(this)->m_diag; }

    size_t m_id = ryml::NONE;
    void align_stream(size_t al) {}

    prefix process_prefix();

    template<size_t Align, class T>
    void read_basic_aligned(T& o) { static_cast<Derived*>(this)->read_basic(o); }
};

template<class Derived>
PULMOTOR_NOINLINE
prefix yaml_archive_read_util<Derived>::process_prefix() {

    yaml_document& doc = yaml();
    ryml::Tree& y = doc.y;

    u32 version=0;

    if (ryml::NodeData const* v = find_with_retry(y, self()->m_id, "_v")) {
        if (v->m_type.has_val())
            if (!ryml::atou(v->m_val.scalar, &version)) {
                diag().report_issue(issue_warning, doc.lookup_location(v->m_val.scalar),
                        ssprintf("invalid version value: %.*s", v->m_val.scalar.len, v->m_val.scalar.str));
                // throw_error(err_invalid_value,
                // 	ssprintf("invalid version value: %.*s", v->m_val.scalar.len, v->m_val.scalar.str).c_str(),
                // 	doc.m_file_name, doc.lookup_location(v->m_val));
                version = 0;
            }
            return prefix { (u16)version, vfl::none };
    } else {
        diag().report_issue(issue_warning, doc.lookup_location(v->m_val.scalar),
            ssprintf("no version value: %.*s", v->m_val.scalar.len, v->m_val.scalar.str));
        version = 0;
    }
    // throw_error(err_key_not_found,
    // 	ssprintf("unable to  version value: %.*s", v->m_val.scalar.len, v->m_val.scalar.str).c_str(),
    // 	doc.m_file_name, doc.lookup_location(v->m_val));

    return prefix{ (u16)version, vfl::none };
}

template<class Derived>
struct archive_yaml_write_util
{
    archive_yaml_write_util(unsigned flags = 0) : m_flags(flags) {}

    Derived& self() { return *static_cast<Derived*>(this); }
    yaml_document& yaml() { return *static_cast<Derived*>(this)->y_doc; }

    size_t m_parent;

    // TODO: assert obj is a "proper" type, for example, not an array
    template<class T>
    void write_object_prefix(T const* obj, unsigned version) {
        u32 vf = version;

        if (obj == nullptr)
            vf |= (u32)vfl::null_ptr;

        std::string name;
        u32 name_length = 0;
        if (m_flags & (unsigned)vfl::debug_string) {
            vf |= (u32)vfl::debug_string;

            name = typeid(T).name();
            name_length = name.size();
            if (name_length > (unsigned)vfl::debug_string_max_size) name_length = (unsigned)vfl::debug_string_max_size;
        }

        ryml::Tree& y = yaml().y;

        char buf[16];
        int bufL = snprintf(buf, sizeof(buf), "0x08", vf);
        size_t i_v = y.append_child(static_cast<Derived*>(this)->m_current);
        y.to_keyval(i_v,
            y.to_arena("_vf"),
            y.to_arena(ryml::csubstr(buf, bufL))
        );

        if (vf & (unsigned)vfl::debug_string) {
            size_t i_d = y.append_child(static_cast<Derived*>(this)->m_current);
            y.to_keyval(i_d,
                y.to_arena("_debug_name"),
                y.to_arena(ryml::csubstr(name.data(), name_length))
            );
        }
    }

    template<size_t Align, class T>
    void write_basic_aligned(T& o) { static_cast<Derived*>(this)->write_basic(o); }

private:
    unsigned m_flags;
};

struct archive_yaml_in
    : archive
    , yaml_archive_read_util<archive_yaml_in>
    , yaml_archive_vu_util<archive_yaml_in>
{
    yaml_document& y_doc;
    size_t m_current;
    diagnostics m_diag;

    archive_yaml_in (yaml_document& doc) : y_doc (doc) {
        m_current = y_doc.y.root_id();
    }

    enum { is_reading = 1, is_writing = 0 };

    template<class T>
    void read_basic(char const* name, size_t name_len, T& a) {
        ryml::Tree const& y = y_doc.y;
        assert( m_current < y.size() );
        size_t curr = m_current;
        ryml::NodeData const* v = find_with_retry(y, m_current, ryml::csubstr(name, name_len));
        if (!v) {
            size_t p = curr == ryml::NONE ? ryml::NONE :
                y.has_parent(curr) ? y.parent(curr) : ryml::NONE;

            // text_location const& loc = p == ryml::NONE ? text_location{0, 0} :
            // 	y.has_key(p) ? y_doc.lookup_location(y.key(p)) :
            // 	y.has_value(ps) ? y_doc.lookup_location(y.key(p)) :
            // 	text_location(y_doc.m_file_name, 0, 0);

            full_location const& loc = y_doc.lookup_location(y.key(p));

            diag().report_issue(issue_warning, loc,
                    ssprintf("no property %.*s found in object", name_len, name));
        }
    }
    void read_data (void* dest, size_t size) { }
};

enum class yaml_data_format
{
    array = 0, base64 = 1
};

/*
struct archive_yaml_out
    : archive
//	, archive_write_util<archive_yaml_out>,
    , archive_yaml_write_util<archive_yaml_out>
    , yaml_archive_vu_util<archive_yaml_out>
//	, archive_pointer_support<archive_sink>
{
    using supported_mods = tl::list<mod_base64, mod_wrap>;

    yaml_document& y_doc;
    size_t m_current;

    archive_yaml_out(yaml_document& ydoc) : y_doc(ydoc) {
        using namespace lit;
        m_current = -1_uz;//y_doc.y.root_id();
    }

    void begin_object (char const* name, size_t name_length) {
        size_t id = y_doc.y.is_root(m_current) ? m_current : y_doc.y.append_child(m_current);

        if (name && name_length)
            y_doc.y.to_map(id, ryml::csubstr(name, name_length), ryml::DOC);
        else
            y_doc.y.to_map(id, ryml::DOC);
        m_current = id;
    }

    void begin_array (char const* name, size_t name_length) {
        bool r = y_doc.y.is_root(m_current);
        size_t id = r ? m_current : y_doc.y.append_child(m_current);
        if (name && name_length)
            y_doc.y.to_seq(id, ryml::csubstr(name, name_length), r ? ryml::DOC : 0);
        else
            y_doc.y.to_seq(id, r ? ryml::DOC : 0);
        m_current = id;
    }

    void end_object() {
        m_current = y_doc.y.parent(m_current);
    }

    template<class T>
    void write_basic(T const& a) {
        // if (m_current == -1) {
        // 	m_current =
        // }
        bool r = y_doc.y.is_root(m_current);
        size_t node = r ? m_current : y_doc.y.append_child(m_current);
        y_doc.y.to_val(node, y_doc.y.to_arena(a), r ? ryml::DOC : 0);
    }

    template<class T>
    void write_basic(char const* name, size_t name_len, T const& a) {
        ryml::Tree& y = y_doc.y;
        bool r = y.is_root(m_current);
        if (r && !y.is_map(m_current)) {
            y.to_map(m_current);
            m_current = y.append_child(m_current);
        }
        size_t child = y_doc.y.append_child(m_current);
        y_doc.y.to_keyval(child,
            y_doc.y.to_arena(ryml::csubstr(name, name_len)),
            y_doc.y.to_arena(a)
        );
    }

    template<class T>
    void write_data_array (T const* arr, size_t size) {
        ryml::Tree& y = y_doc.y;
        bool r = y.is_root(m_current);
        if (r && !y.is_seq(m_current)) {
                m_current = y.append_child(m_current);
            y.to_map(m_current);
        }
        size_t child = y_doc.y.append_child(m_current);
        for (size_t i=0; i<size; ++i) {
            size_t el_child = y_doc.y.append_child(m_current);
            T const& v = arr[i];
            y_doc.y.to_val(el_child, y_doc.y.to_arena(v));
        }
    }

    void write_data_blob (char const* arr, size_t size) {
        size_t child = y_doc.y.append_child(m_current);
        ryml::NodeRef r = y_doc.y.ref(child);
        auto base64wrap = c4::fmt::cbase64(arr, size);
        y_doc.y.to_val(child, y_doc.y.to_arena(base64wrap));
    }

    void write_data_blob (char const* name, size_t name_len, char const* arr, size_t size) {
        size_t child = y_doc.y.append_child(m_current);
        ryml::NodeRef r = y_doc.y.ref(child);
        auto base64wrap = c4::fmt::cbase64(arr, size);
        y_doc.y.to_keyval(child,
            y_doc.y.to_arena(ryml::csubstr(name, name_len)),
            y_doc.y.to_arena(base64wrap)
        );
    }
};
*/

// int a; ar | a | b;
//  --> stream per value: --- a / --- b
//
// S s, t; ar | s | t;
//  --> stream per value: --- s: { ... } / --- t: { ... }
//
// int a, b; S s, t; ar | a | nv("b", b) | s | nv("t", t);
//  --> stream per value, implicit mapping: --- a / --- b: b / --- s / --- t: { ... }
//
// int a, b; ar | nv("a", a) | nv("b", b);
//  --> implicit mapping: a: ... / b: ...
//
// S s; ar | nv("a", s);
//  --> implicit mapping and then the mapping for s: a: { ... }

//
/*
struct archive_yaml_writer
{
    using supported_mods = tl::list<mod_base64, mod_wrap>;

    struct context {
        u8 yaml_flags = 0;
        u8 base64:1;
        u8 base64_wrap = 0;
    };

    archive_yaml_writer(sink& s) : m_y(s)
    {}

    void start_stream() {
        if (!m_had_stream)
            m_had_stream = true;
        else
            m_y.new_stream();
    }

    template<class T>
    void write_single(T const& a, context const& ctx) {
        if (m_level == 0) start_stream();
        yaml::encoder::format_value(m_y, a, ctx.yaml_flags);
        if (m_implicit_mapping)
            end_container();
    }

    void begin_object (context const& ctx) {
        if (m_level == 0) start_stream();
        m_y.mapping(ctx.yaml_flags); // flow, block etc.
        ++m_level;
    }

    void write_key(std::string_view s, context const& ctx) {
        if (m_level == 0) {
            start_stream();
            m_y.mapping(ctx.yaml_flags); // flow, block etc.
            m_implicit_mapping = true;
            ++m_level;
        }
        yaml::encoder::format_key(m_y, s);
    }

    void begin_array (context const& ctx) {
        if (m_level == 0)
            start_stream();
        m_y.sequence(ctx.yaml_flags);
        ++m_level;
    }

    template<class T>
    void encode_array(context const& ctx, T const* p, size_t size) {
        size_t s = size * sizeof(T);
        yaml::encoder::encode_value(m_y, (char const*)p, s, ctx.base64_wrap);
    }

    void end_container() {
        m_y.end_container();
        --m_level;
    }

    enum { dash_first_stream = 1 };
    void set_flags( unsigned flags) {
        if (flags & dash_first_stream)
            m_had_stream = true;
    }

private:
    yaml::writer m_y;
    size_t m_level = 0;
    bool m_had_stream = false;
    bool m_implicit_mapping = false;
};*/

struct yaml_base64;

struct archive_yaml
{
    enum flags
    {
        none = 0,
        no_stream = 0x08,
        keyed = 0x01
    };

    enum modes
    {
        base64 = 1
    };
};
inline constexpr archive_yaml::flags operator|(archive_yaml::flags a, archive_yaml::flags b) { return (archive_yaml::flags)((int)a | (int)b); }

struct archive_write_yaml : public archive
{
    yaml::writer1& m_writer;
    archive_yaml::flags m_flags;

    using supported_mods = tl::list<yaml_base64>;

    enum { is_reading = 0, is_writing = 1 };

    enum state
    {
        st_clear  = 0,
        st_pending_prepare = 0x01,
        st_base64 = 0x02,
        // st_primitive = 0x20,
        st_mapping = 0x80,
        st_array = 0x40,
    };
    struct scope {
        u16 wrap = 0;
        u8 flags = 0; // fmt_flags
        u8 state = 0;
        prefix obj_prefix;
        bool base64 = false;
    };
    scope m_scope = scope{};
    std::vector<scope> m_sstack;

    std::string m_object_debug_str;

    // int m_written = 0;

    archive_write_yaml(yaml::writer1& w, archive_yaml::flags flags = archive_yaml::none);
    ~archive_write_yaml();

    // void start_stream();

    scope& current_scope() { return m_scope; }
    void restore_scope(scope const& s) { m_scope = s; }

    scope fresh_scope() { return scope{}; }

    void begin_object();
    void end_object();
    void begin_array(size_t& size);
    void end_array();

    template<class T>
    void write_named(std::string_view name, T const& value) {
        assert (m_flags & archive_yaml::keyed);
        write_key(name);
        *this | value;
    }

    void prepare_container(scope& sc);

    template<std::integral T>
    void write_single (T const& data) {
        if constexpr(sizeof(T) <= sizeof(int))
            write_int(data);
        else
            write_longlong(data);
    }

    template<std::floating_point T>
    void write_single (T const& data) {
        write_float(data);
    }

    void write_single (std::string const& data) { write_string(data); }

    template<class T, unsigned Mode = 0>
    void write_data (T const* src, size_t size) {
        if constexpr ((Mode & 0x01) == (unsigned)yaml_data_format::base64) {
            encode_base64(src, size, (Mode & 0xff00) >> 8);
        } else {
            if constexpr (sizeof(T) == 1)
                write_byte_array(src, size);
            else
                write_array(src, size);
        }
    }

    void write_prefix(prefix pre, prefix_ext const* ppx = nullptr);

protected:
    void actually_write_prefix(scope& sc);
    void actually_prepare_array(scope& sc);

    void write_key(std::string_view key);

    void write_string(std::string_view s);

    void write_int(int value);
    void write_longlong(long long value);
    void write_float(double value);

    void write_byte_array(u8 const* data, size_t count);
    void write_array(bool const* data, size_t count);
    void write_array(u16 const* data, size_t count);
    void write_array(s16 const* data, size_t count);
    void write_array(u32 const* data, size_t count);
    void write_array(s32 const* data, size_t count);
    void write_array(u64 const* data, size_t count);
    void write_array(s64 const* data, size_t count);
    void write_array(float const* data, size_t count);
    void write_array(double const* data, size_t count);
    void write_array(std::string const* data, size_t count);

    void encode_base64(void const* data, size_t size, size_t wrap);

    template<class T>
    void write_array_impl(T const* data, size_t count);
 };

template<class Ch, class Tr, class Al>
struct is_primitive_type<archive_write_yaml, std::basic_string<Ch, Tr, Al>> : std::integral_constant<bool, true> {};

struct yaml_base64 : mod_op<yaml_base64> {
    u8 wrap = 0;
    constexpr void operator()(archive_write_yaml::scope& s) {
        s.state |= archive_write_yaml::st_base64;
        s.wrap = wrap;
    }
};

struct yaml_fmt : mod_op<yaml_fmt> {
    yaml::fmt_flags flags;

    yaml_fmt(yaml::fmt_flags fl) : flags(fl) {}
    constexpr void operator()(archive_write_yaml::scope& s) const {
        s.flags |= flags;
    }
};


struct archive_read_yaml
{
    yaml_document& y_doc;
    size_t m_current;
    // diagnostics m_diag;

    archive_read_yaml (yaml_document& doc) : y_doc (doc) {
        m_current = y_doc.y.root_id();
    }

    void begin_object() {
        m_current = y_doc.y.first_child(m_current);
    }
    void end_object() {
        m_current = y_doc.y.parent(m_current);
    }

    void begin_array(size_t& size) {
        m_current = y_doc.y.first_child(m_current);
        size = y_doc.y.num_siblings(m_current);
    }
    void end_array() {
        m_current = y_doc.y.parent(m_current);
    }

    template<std::integral T>
    void read_named(std::string_view name, T& value) {
        read_integral(name, value);
    }

    template<std::floating_point T>
    void read_named(std::string_view name, T& value) {
        read_float(name, value);
    }

    template<class T, class Y, class Tr, class Al>
        requires (std::is_same_v<std::basic_string<Y, Tr, Al>, T>)
    void read_named(std::string_view name, T& value) {
        read_string(name, value);
    }

    template<class T>
    void read_single(T& data) {
        // ryml::Tree const& y = y_doc.y;
        // assert( m_current < y.size() );
        // size_t curr = m_current;
        // ryml::NodeData const* v = find_with_retry(y, m_current, ryml::csubstr(name, name_len));
        // ryml::from_chars(v->m_val.scalar, &data);
    }

    size_t get_array_size() {
        return y_doc.y.num_children(m_current);
    }

    template<class T>
    void read_data (T* dest, size_t size) {
    }

    prefix read_prefix(prefix_ext* ext);

private:
    ryml::NodeData const* find_node(std::string_view name);

    void read_string(std::string_view name, std::string_view s);
    void read_integral(std::string_view name, int value);
    void read_integral(std::string_view name, long long value);
    void read_float(std::string_view name, float& value);
    void read_float(std::string_view name, double& value);

    void read_byte_array(u8* data);
    void read_array(bool* data);
    void read_array(s16* data);
    void read_array(u16* data);
    void read_array(u32* data);
    void read_array(s32* data);
    void read_array(u64* data);
    void read_array(s64* data);
    void read_array(float* data);
    void read_array(double* data);

};

template<class Ch, class Tr, class Al>
struct is_primitive_type<archive_read_yaml, std::basic_string<Ch, Tr, Al>> : std::integral_constant<bool, true> {};

// struct moo
// {
// 	int x, y;
//	char buffer[256];
// 	template<class Ar> void serialize(Ar& ar) {
//		ar | x | y;
//		ar | buffer * wrap(64) * base64;
// 	}

// 	template<class Ar> void serialize(Ar& ar) {
// 		ar	| nv("x", x)
// 			| nv("y", y);
// 	}
// };

} // pulmotor

#endif // PULMOTOR_YAML_ARCHIVE_HPP
