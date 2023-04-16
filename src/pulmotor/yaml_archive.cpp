#include "yaml_archive.hpp"
#include "yaml_emitter.hpp"

namespace pulmotor {

#define PULMOTOR_YAML_CONTINUE_FROM_DISCOVERED_PROPERTY 1
ryml::NodeData const* find_with_retry(ryml::Tree const& y, size_t& cur_id, ryml::csubstr const& prop_name)
{
    ryml::NodeData const* n = y.get(cur_id);

    if (!n->m_type.has_key())
        return nullptr;

    if (n->m_key.scalar != prop_name) {
        size_t pid = y.prev_sibling(cur_id);
        size_t nid = y.next_sibling(cur_id);
        while (pid != ryml::NONE && nid != ryml::NONE) {
            if (pid != ryml::NONE) {
                ryml::NodeData const* pn = y.get(pid);
                if (pn->m_type.has_key() && pn->m_key.scalar == prop_name) {
#if PULMOTOR_YAML_CONTINUE_FROM_DISCOVERED_PROPERTY
                    cur_id = y.next_sibling(pid);
#endif
                    return pn;
                }
                pid = y.prev_sibling(pid);
            }
            if (nid != ryml::NONE) {
                ryml::NodeData const* nn = y.get(nid);
                if (nn->m_type.has_key() && nn->m_key.scalar == prop_name) {
                    n = nn;
#if PULMOTOR_YAML_CONTINUE_FROM_DISCOVERED_PROPERTY
                    cur_id = y.next_sibling(nid);
#endif
                    return nn;
                }
                nid = y.next_sibling(nid);
            }
        }
        return nullptr;
    } else {
        size_t id = cur_id;
        cur_id = y.next_sibling(cur_id);
        return n;
    }
}

yaml_document::yaml_document(char const* name)
:   m_file_name(name) {
}

yaml_document::~yaml_document() {
}

full_location yaml_document::lookup_location( ryml::csubstr const& s ) {
    size_t offset = s.data() - m_text_start;
    return full_location { m_file_name, loc.has_value() ? loc->lookup(offset) : text_location{} };
}

void yaml_document::ryml_error( const char* msg, size_t msg_len, ryml::Location location ) {
    text_location l { (unsigned)location.line, (unsigned)location.col };
    std::string msgS(msg, msg_len), fn{ location.name.str, location.name.len };
    throw_error(err_unspecified, msgS.c_str(), fn.c_str(), l);
}

void yaml_document::parse( ryml::substr s, path_char const* filename, bool index_lines ) {
    ryml::Parser p(m_callbacks);
    if (index_lines) {
        loc.emplace( s.data(), s.size() );
        loc->analyze();
        y.reserve( loc->line_count() + 16);
    } else {
        size_t lines = s.count('\n');
        y.reserve(lines >= 16 ? lines : 16 );
    }

    p.parse_in_place( ryml::to_csubstr(filename), s, &y );
}

std::string yaml_document::to_string() {
    return ryml::emitrs_yaml<std::string>(y);
}


// int a; ar | a | b;
// --> --- a / --- b
//
// S s, t; ar | s | t;
// --> --- [ s.a, s.b ] / --- t
//
// int a, b; S s, t; ar | a | nv("b", b) | s | nv("t", t);
// --> POSSIBLE ERROR: can't mix named and unnamed in the same stream!
//
// int a, b; ar | nv("a", a) | nv("b", b);
// int a, b; ar | "a"_nv + a | "b"_nv + b;
// --> { a: a, b: b }
//
// S s; ar | nv("a", s);
// --> { a: { sx: s.x, sy: s.y } }

archive_write_yaml::archive_write_yaml(yaml::writer1& w, archive_yaml::flags flags)
    : m_writer(w), m_flags(flags) {
    m_sstack.reserve(8);
    // if ( !(m_flags & archive_yaml::no_stream) )
    //     m_writer.new_stream();

    // if ( (m_flags & archive_yaml::keyed) )
    //     m_writer.mapping(m_scope.flags);
    // else
    //     m_writer.sequence(m_scope.flags);
}

archive_write_yaml::~archive_write_yaml() {
    // m_writer.end_collection();
}

// void archive_write_yaml::start_stream() {
//     m_writer.new_stream();
// }

void archive_write_yaml::begin_object() {
    if (m_scope.state & st_pending_prepare)
        prepare_container(m_scope);
    m_sstack.push_back(m_scope);
    m_scope = fresh_scope();
    m_scope.state |= st_pending_prepare|st_mapping;
}

void archive_write_yaml::end_object() {
    m_writer.end_collection();
    m_scope = m_sstack.back();
    m_sstack.pop_back();
}

void archive_write_yaml::begin_array(size_t&) {
    if (m_scope.state & st_pending_prepare)
        prepare_container(m_scope);

    m_sstack.push_back(m_scope);
    m_scope = fresh_scope();
    m_scope.state |= st_pending_prepare|st_array;
    // if (primitive) m_scope.state |= st_primitive;
}

void archive_write_yaml::end_array() {
    m_writer.end_collection(); // end array
    // bool written = m_scope.obj_prefix.store() && !m_scope.obj_prefix.is_default_version();
    // if (written && ! (m_scope.state & st_primitive))
    //     m_writer.end_collection(); // end mapping
    m_scope = m_sstack.back();
    m_sstack.pop_back();
}

void archive_write_yaml::prepare_container(scope& sc)
{
    if (sc.state & st_pending_prepare) {
        if (sc.state & st_mapping) {
            m_writer.mapping((yaml::fmt_flags)sc.flags);
            if (sc.obj_prefix.store() && !sc.obj_prefix.is_default_version())
                actually_write_prefix(sc);
        } else if (sc.state & st_array) {
            actually_prepare_array(sc);
        }
        sc.state &= ~st_pending_prepare;
    }
}


void archive_write_yaml::actually_prepare_array(scope& sc)
{
    // bool write = sc.obj_prefix.store() && !sc.obj_prefix.is_default_version();

    // if (!write || (sc.state & st_primitive)) {
        m_writer.sequence((yaml::fmt_flags)sc.flags);
    // } else {
    //     m_writer.mapping(sc.flags);
    //     m_writer.key("meta");
    //         m_writer.mapping(sc.flags);
    //         actually_write_prefix(sc);
    //         m_writer.end_collection();
    //     m_writer.key("data");
    // }
}

void archive_write_yaml::write_prefix(prefix pre, prefix_ext const* ppx) {
    m_scope.obj_prefix = pre;
    if (ppx)
        m_object_debug_str = ppx->class_name;
    else
        m_object_debug_str.clear();
}

// position: { version: 2, x: 10, y: 20, z: 30 }
// position:
//     pm_version: 2
//     pm_flags: 0x2323
// position: { pulM: { version: 2, flags: 20 }, x: 10, y: 20, z: 30 }

// data:
//   -
//     x: 3
//     y: 6
//     z: 9
//   -
//     x: 3
//     y: 6
//     z: 9
//   -
//     x: 3
//     y: 6
//     z: 9

// data:
//   - hello
//   - moo
//   - back

// data:
//   meta:
//     version: 3
//     flags: 5
//   content:
//     -
//       x: 3
//       y: 6
//       z: 9
//     -
//       x: 3
//       y: 6
//       z: 9
//     -
//       x: 3
//       y: 6
//       z: 9


void archive_write_yaml::actually_write_prefix(scope& sc)
{
    if (sc.obj_prefix.store() && !sc.obj_prefix.is_default_version()) {
        yaml::encoder::format_key(m_writer, PULMOTOR_TEXT_INTERNAL_PREFIX "_version");
        yaml::encoder::format_value(m_writer, sc.obj_prefix.version);
    }
}

void archive_write_yaml::write_key(std::string_view key) {
    prepare_container(m_scope);
    yaml::encoder::format_key(m_writer, key);
}

template<class T>
void archive_write_yaml::write_array_impl(T const* data, size_t count)
{
    for(size_t i=0; i<count; ++i)
        yaml::encoder::format_value(m_writer, data[i], yaml::fmt_flags::fmt_none);
}

void archive_write_yaml::write_int(int value) {
    prepare_container(m_scope);
    yaml::encoder::format_value(m_writer, value);
}
void write_longlong(long long value);
void archive_write_yaml::write_float(double value) {
    prepare_container(m_scope);
}

void archive_write_yaml::write_string(std::string_view s) {
    prepare_container(m_scope);
    m_writer.value(s, (yaml::fmt_flags)m_scope.flags);
}

void archive_write_yaml::write_byte_array(u8 const* data, size_t count) {
    prepare_container(m_scope);
    if (m_scope.state & st_base64)
        yaml::encoder::encode_value(m_writer, (char const*)data, count, m_scope.wrap);
    else
        write_array_impl(data, count);
}
void archive_write_yaml::write_array(bool const* data, size_t count) {
    prepare_container(m_scope);
    write_array_impl(data, count);
}
void archive_write_yaml::write_array(s16 const* data, size_t count) {
    prepare_container(m_scope);
    write_array_impl(data, count);
}
void archive_write_yaml::write_array(u16 const* data, size_t count) {
    prepare_container(m_scope);
    write_array_impl(data, count);
}
void archive_write_yaml::write_array(s32 const* data, size_t count) {
    prepare_container(m_scope);
    write_array_impl(data, count);
}
void archive_write_yaml::write_array(u32 const* data, size_t count) {
    prepare_container(m_scope);
    write_array_impl(data, count);
}
void archive_write_yaml::write_array(u64 const* data, size_t count) {
    prepare_container(m_scope);
    write_array_impl(data, count);
}
void archive_write_yaml::write_array(s64 const* data, size_t count) {
    prepare_container(m_scope);
    write_array_impl(data, count);
}
void archive_write_yaml::write_array(float const* data, size_t count) {
    prepare_container(m_scope);
    write_array_impl(data, count);
}
void archive_write_yaml::write_array(double const* data, size_t count) {
    prepare_container(m_scope);
    write_array_impl(data, count);
}
void archive_write_yaml::write_array(std::string const* data, size_t count) {
    prepare_container(m_scope);
    write_array_impl(data, count);
}
void archive_write_yaml::encode_base64(void const* data, size_t size, size_t wrap) {
    yaml::encoder::encode_value(m_writer, (char const*)data, size, wrap);
}


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
