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

archive_write_yaml::archive_write_yaml(yaml::writer& w, archive_yaml::flags flags)
    : m_writer(w), m_flags(flags) {
    m_sstack.reserve(8);
    if ( !(m_flags & archive_yaml::no_stream) )
        m_writer.new_stream();
}

void archive_write_yaml::start_stream() {
    m_writer.new_stream();
}

void archive_write_yaml::begin_object() {
    m_writer.mapping(m_state.flags);
}
void archive_write_yaml::end_object() {
    m_writer.end_container();
}

void archive_write_yaml::begin_array() {
    m_writer.mapping(m_state.flags);
}
void archive_write_yaml::end_array() {
    m_writer.end_container();
}

void archive_write_yaml::write_prefix(prefix pre, prefix_ext const* ppx) {
}

void archive_write_yaml::write_key(std::string_view key) {
    yaml::encoder::format_key(m_writer, key);
}

void archive_write_yaml::write_int(int value) {
    yaml::encoder::format_value(m_writer, value);
}
void write_longlong(long long value);
void archive_write_yaml::write_float(double value) {
}

void archive_write_yaml::write_string(std::string_view s) {
    yaml::encoder::format_value(m_writer, s);
}


void write_byte_array(u8 const* data, size_t count) {
}
void write_array(bool const* data, size_t count);
void write_array(s16 const* data, size_t count);
void write_array(u16 const* data, size_t count);
void write_array(u32 const* data, size_t count);
void archive_write_yaml::write_array(s32 const* data, size_t count) {
}
void write_array(u64 const* data, size_t count);
void write_array(s64 const* data, size_t count);
void write_array(float const* data, size_t count);
void write_array(double const* data, size_t count);

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
