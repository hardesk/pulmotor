#include "yaml_archive.hpp"
#include "yaml_emitter.hpp"

namespace pulmotor {

#define PULMOTOR_YAML_CONTINUE_FROM_DISCOVERED_PROPERTY 1
ryml::NodeData const* find_with_retry(ryml::Tree const& y, size_t& cur_id, ryml::csubstr const& prop_name)
{
    ryml::NodeData const* n = y.get(cur_id);
    if (!n) return nullptr;

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
        // cur_id = y.next_sibling(cur_id);
        return n;
    }
}

// void next_sibling(ryml::Tree const& y, size_t& cur_id)
// {
//     cur_id = y.next_sibling(cur_id);
// }


yaml_document::yaml_document() {
}

yaml_document::yaml_document(path_char const* name)
:   m_file_name(name) {
}

yaml_document::~yaml_document() {
}

full_location yaml_document::lookup_location( ryml::csubstr const& s ) {
    if (!m_text_start)
        return full_location { m_file_name.c_str(), 0, 0 };
    size_t offset = s.data() - m_text_start;
    return full_location { m_file_name.c_str(), loc.has_value() ? loc->lookup(offset) : text_location{} };
}

void yaml_document::ryml_error( const char* msg, size_t msg_len, ryml::Location location ) {
    text_location l { (unsigned)location.line, (unsigned)location.col };
    std::string msgS(msg, msg_len), fn{ location.name.str, location.name.len };
    throw_error(err_unspecified, msgS.c_str(), fn.c_str(), l);
}

void yaml_document::parse( ryml::substr s, path_char const* filename, bool index_lines ) {
    ryml::Parser p(m_callbacks);
    m_file_name = filename;
    m_text_start = s.data();
    if (index_lines) {
        loc.emplace( s.data(), s.size() );
        loc->analyze();
        y.reserve( loc->line_count() + 16);
    } else {
        size_t lines = s.count('\n');
        y.reserve(lines >= 16 ? lines : 16 );
    }

    p.parse_in_place( ryml::to_csubstr(m_file_name), s, &y );
}

std::string yaml_document::to_string() {
    return ryml::emitrs_yaml<std::string>(y);
}

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

// What happens here is that we want to delay "prepare container" until the first member is serialized. That is because
// the style of the serialization (block vs flow) is only specified in the serialization function of the object itself, eg.
// ar | x; // where x has the following serialization function
// ar | yaml_fmt(yaml::flow) | member;
// In this case container style is only known before serializing 'member'.
// The same goes for the arrays. The style is specified after begin_array.
void archive_write_yaml::begin_object(prefix const& px, prefix_ext* pxx) {
    prepare_container(m_scope);
    m_sstack.push_back(m_scope);
    m_scope = fresh_scope();
    m_scope.state |= st_pending_prepare|st_mapping;
    m_scope.obj_prefix = px;
    if (pxx)
        m_object_debug_str = pxx->class_name;
    else
        m_object_debug_str.clear();
}

void archive_write_yaml::end_object() {
    m_writer.end_collection();
    m_scope = m_sstack.back();
    m_sstack.pop_back();
}

void archive_write_yaml::begin_array(size_t const&) {
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
        bool degrade = (sc.state & st_mapping) && !(sc.state & st_key);
        if (degrade) {
            sc.state &= ~st_mapping;
            sc.state |= st_array;
        }

        if (sc.state & st_array) {
            // actually_prepare_array(sc);
            m_writer.sequence((yaml::fmt_flags)sc.flags);
            sc.state |= st_deny_key;
        } else if (sc.state & st_mapping) {
            m_writer.mapping((yaml::fmt_flags)sc.flags);
            // do not write version if it's default
            if (sc.obj_prefix.store() && !sc.obj_prefix.is_default_version())
                actually_write_prefix(sc);
            sc.state |= st_expect_key;
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


void archive_write_yaml::actually_write_prefix(scope& sc) {
    if (sc.obj_prefix.store() && !sc.obj_prefix.is_default_version()) {
        yaml::encoder::format_key(m_writer, PULMOTOR_TEXT_INTERNAL_PREFIX "version");
        yaml::encoder::format_value(m_writer, sc.obj_prefix.version);
    }
}

void archive_write_yaml::write_key(std::string_view key) {
    if (!(m_scope.state & st_mapping)) PULMOTOR_THROW_FMT_ERROR("key %s unexpected in non-mapping");

    // mark that we'll write key for prepare container to create a map
    m_scope.state |= st_key;
    prepare_container(m_scope);
    yaml::encoder::format_key(m_writer, key);
}

template<class T>
void archive_write_yaml::write_array_impl(T const* data, size_t count) {
    if (!(m_scope.state & st_array)) PULMOTOR_THROW_FMT_ERROR("attempting to write array data into non-array");
    for(size_t i=0; i<count; ++i)
        yaml::encoder::format_value(m_writer, data[i], yaml::fmt_flags::fmt_none);
    m_scope.children_written += count;
    m_scope.state &= st_key;
}

void archive_write_yaml::write_char(char value) {
    prepare_container(m_scope);
    yaml::encoder::format_value(m_writer, value);
    m_scope.children_written++;
    m_scope.state &= st_key;
}
void archive_write_yaml::write_int(int value) {
    prepare_container(m_scope);
    yaml::encoder::format_value(m_writer, value);
    m_scope.children_written++;
    m_scope.state &= st_key;
}
void archive_write_yaml::write_uint(unsigned value) {
    prepare_container(m_scope);
    yaml::encoder::format_value(m_writer, value);
    m_scope.children_written++;
    m_scope.state &= st_key;
}
void archive_write_yaml::write_longlong(long long value) {
    prepare_container(m_scope);
    yaml::encoder::format_value(m_writer, value);
    m_scope.children_written++;
    m_scope.state &= st_key;
}
void archive_write_yaml::write_ulonglong(unsigned long long value) {
    prepare_container(m_scope);
    yaml::encoder::format_value(m_writer, value);
    m_scope.children_written++;
    m_scope.state &= st_key;
}
void archive_write_yaml::write_float(double value) {
    prepare_container(m_scope);
    m_scope.children_written++;
    m_scope.state &= st_key;
}

void archive_write_yaml::write_string(std::string_view s) {
    prepare_container(m_scope);
    // assert( ((m_scope.state & st_array) && (m_scope.state & st_deny_key)) || (m_scope.state & st_deny_key) );

    yaml::fmt_flags f = (yaml::fmt_flags)m_scope.flags;
    if (s.empty())
        f |= yaml::fmt_flags::single_quoted;
    m_writer.value(s, f);
    m_scope.children_written++;
    m_scope.state &= st_key;
}

void archive_write_yaml::write_byte_array(u8 const* data, size_t count) {
    prepare_container(m_scope);
    if (m_scope.state & st_base64) {
        yaml::encoder::encode_value(m_writer, (char const*)data, count, m_scope.wrap);
        m_scope.children_written++;
    } else {
        write_array_impl(data, count);
        m_scope.children_written += count;
    }
    m_scope.state &= st_key;
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
    m_scope.state &= ~st_key;
}

// YAML KEYED

archive_write_yaml_keyed::archive_write_yaml_keyed(yaml::writer1& w, archive_yaml::flags flags)
    : m_writer(w), m_flags(flags) {
    m_sstack.reserve(8);
    // if ( !(m_flags & archive_yaml::no_stream) )
    //     m_writer.new_stream();
    // if ( (m_flags & archive_yaml::keyed) )
    //     m_writer.mapping(m_scope.flags);
    // else
    //     m_writer.sequence(m_scope.flags);
}

archive_write_yaml_keyed::~archive_write_yaml_keyed() {
    // m_writer.end_collection();
}

// void archive_write_yaml_keyed::start_stream() {
//     m_writer.new_stream();
// }

// What happens here is that we want to delay "prepare container" until the first member is serialized. That is because
// the style of the serialization (block vs flow) is only specified in the serialization function of the object itself, eg.
// ar | x; // where x has the following serialization function
// ar | yaml_fmt(yaml::flow) | member;
// In this case container style is only known before serializing 'member'.
// The same goes for the arrays. The style is specified after begin_array.
void archive_write_yaml_keyed::begin_object(prefix const& px, prefix_ext* pxx) {

    bool store_version = (px.store() && !px.is_default_version()) || px.flags != vfl::none;

    if (store_version)
        m_writer.tag("!<pulmotor>:versioned");

    m_writer.mapping((yaml::fmt_flags)m_scope.flags);
    m_sstack.push_back(m_scope);
    m_scope = fresh_scope();

    // do not write version if it's default
    if (store_version) {
        u32 vf = px.combine();
        yaml::encoder::format_key(m_writer, PULMOTOR_TEXT_INTERNAL_PREFIX "_version_flag");
        yaml::encoder::format_value(m_writer, vf);
    }
}

void archive_write_yaml_keyed::end_object() {
    m_writer.end_collection();
    m_scope = m_sstack.back();
    m_sstack.pop_back();
}

void archive_write_yaml_keyed::begin_array(size_t const&) {
    m_writer.sequence((yaml::fmt_flags)m_scope.flags);
    m_sstack.push_back(m_scope);
    m_scope = fresh_scope();
}

void archive_write_yaml_keyed::end_array() {
    m_writer.end_collection();
    m_scope = m_sstack.back();
    m_sstack.pop_back();
}

void archive_write_yaml_keyed::write_key(std::string_view key) {
    yaml::encoder::format_key(m_writer, key);
}

template<class T>
void archive_write_yaml_keyed::write_array_impl(T const* data, size_t count)
{
    for(size_t i=0; i<count; ++i)
        yaml::encoder::format_value(m_writer, data[i], yaml::fmt_flags::fmt_none);
}

void archive_write_yaml_keyed::write_char(char value) {
    yaml::encoder::format_value(m_writer, value);
}
void archive_write_yaml_keyed::write_int(int value) {
    yaml::encoder::format_value(m_writer, value);
}
void archive_write_yaml_keyed::write_uint(unsigned value) {
    yaml::encoder::format_value(m_writer, value);
}
void archive_write_yaml_keyed::write_longlong(long long value) {
    yaml::encoder::format_value(m_writer, value);
}
void archive_write_yaml_keyed::write_ulonglong(unsigned long long value) {
    yaml::encoder::format_value(m_writer, value);
}
void archive_write_yaml_keyed::write_float(double value) {
    yaml::encoder::format_value(m_writer, value);
}

void archive_write_yaml_keyed::write_string(std::string_view s) {
    // if (s.empty())
    //     m_writer.value("~", (yaml::fmt_flags)m_scope.flags);
    // else
        m_writer.value(s, (yaml::fmt_flags)m_scope.flags);
}

void archive_write_yaml_keyed::write_byte_array(u8 const* data, size_t count) {
    if (m_scope.base64)
        yaml::encoder::encode_value(m_writer, (char const*)data, count, m_scope.wrap);
    else
        write_array_impl(data, count);
}
void archive_write_yaml_keyed::write_array(bool const* data, size_t count) {
    write_array_impl(data, count);
}
void archive_write_yaml_keyed::write_array(s16 const* data, size_t count) {
    write_array_impl(data, count);
}
void archive_write_yaml_keyed::write_array(u16 const* data, size_t count) {
    write_array_impl(data, count);
}
void archive_write_yaml_keyed::write_array(s32 const* data, size_t count) {
    write_array_impl(data, count);
}
void archive_write_yaml_keyed::write_array(u32 const* data, size_t count) {
    write_array_impl(data, count);
}
void archive_write_yaml_keyed::write_array(u64 const* data, size_t count) {
    write_array_impl(data, count);
}
void archive_write_yaml_keyed::write_array(s64 const* data, size_t count) {
    write_array_impl(data, count);
}
void archive_write_yaml_keyed::write_array(float const* data, size_t count) {
    write_array_impl(data, count);
}
void archive_write_yaml_keyed::write_array(double const* data, size_t count) {
    write_array_impl(data, count);
}
void archive_write_yaml_keyed::write_array(std::string const* data, size_t count) {
    write_array_impl(data, count);
}
void archive_write_yaml_keyed::encode_base64(void const* data, size_t size, size_t wrap) {
    yaml::encoder::encode_value(m_writer, (char const*)data, size, wrap);
}


// YAML READ

void archive_read_yaml::begin_object(prefix const& px, prefix_ext* pxx) {

    assert(m_scope.id != ryml::NONE);

    // enter the container
    m_sstack.push_back(m_scope);
    m_scope.id = y_doc.y.first_child(m_scope.id);

    bool extract = false;
    ryml::NodeData const* n0 = y_doc.y.get(m_scope.id);
    if (n0 && n0->m_type.is_container() && n0->m_val.tag=="!<pulmotor>:versioned")
        extract = true;

    if (extract) {
        ryml::NodeData const* n1 = find_with_retry(y_doc.y, m_scope.id, c4::to_csubstr(PULMOTOR_TEXT_INTERNAL_PREFIX "_version_flag"));
        if (n1) {
            u32 vf = 0;
            if (!c4::from_chars(n1->m_val.scalar, &vf)) {
                full_location l = y_doc.lookup_location(n1->m_val.scalar);
                PULMOTOR_THROW_FMT_ERROR("Unable to interpret '%.*s' as version at %s:%d.",
                    n1->m_val.scalar.size(), n1->m_val.scalar.data(), l.name, l.line);
            }
            const_cast<prefix&>(px) = prefix::from_u32(vf);
            if (size_t nx = y_doc.y.next_sibling(m_scope.id); nx != ryml::NONE)
                m_scope.id = nx;
        }

        if (pxx && px.has_garbage()) {
            ryml::NodeData const* n3 = find_with_retry(y_doc.y, m_scope.id, c4::to_csubstr(PULMOTOR_TEXT_INTERNAL_PREFIX "_class_name"));
            if (n3) {
                if (!c4::from_chars(n3->m_val.scalar, &pxx->class_name)) {
                    full_location l = y_doc.lookup_location(n3->m_val.scalar);
                    PULMOTOR_THROW_FMT_ERROR("Unable to interpret '%.*s' as class name at %s:%d.",
                        n3->m_val.scalar.size(), n3->m_val.scalar.data(), l.name, l.line);
                }
                if (size_t nx = y_doc.y.next_sibling(m_scope.id); nx != ryml::NONE)
                    m_scope.id = nx;
            }
        }
    }
}

void archive_read_keyed_yaml::begin_object(prefix const& px, prefix_ext* pxx) {
    assert(m_scope.id != ryml::NONE);

    if (m_diag)
        log_action(y_doc.y.get(m_scope.id), "ENTER"), m_diag->m_depth++;

    m_sstack.push_back(m_scope);
    m_scope.id = y_doc.y.first_child(m_scope.id);

    bool extract = false;
    ryml::NodeData const* n0 = y_doc.y.get(m_scope.id);
    if (n0 && n0->m_type.is_container() && n0->m_val.tag=="!<pulmotor>:versioned")
        extract = true;

    if (extract) {
        auto throwup = [this](char const* fmt, c4::csubstr scalar) {
            full_location l = y_doc.lookup_location(scalar);
            PULMOTOR_THROW_FMT_ERROR(fmt, scalar.size(), scalar.data(), l.name, l.line);
        };

        u32 vf = 0;
        ryml::NodeData const* n1 = find_with_retry(y_doc.y, m_scope.id, c4::to_csubstr(PULMOTOR_TEXT_INTERNAL_PREFIX "_version_flag"));
        if (n1) {
            if (!c4::from_chars(n1->m_val.scalar, &vf))
                throwup("Unable to interpret '%.*s' as version/flags at %s:%d.", n1->m_val.scalar);
            const_cast<prefix&>(px) = prefix::from_u32(vf);
            if (size_t nx = y_doc.y.next_sibling(m_scope.id); nx != ryml::NONE)
                m_scope.id = nx;
        }

        if (pxx) {
            ryml::NodeData const* n3 = find_with_retry(y_doc.y, m_scope.id, c4::to_csubstr(PULMOTOR_TEXT_INTERNAL_PREFIX "class_name"));
            if (n3) {
                if (!c4::from_chars(n1->m_val.scalar, &pxx->class_name))
                    throwup("Unable to interpret '%.*s' as class name at %s:%d.", n1->m_val.scalar);
            if (size_t nx = y_doc.y.next_sibling(m_scope.id); nx != ryml::NONE)
                m_scope.id = nx;
            }
        }
    }
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
