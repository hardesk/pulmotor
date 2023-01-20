#ifndef PULMOTOR_YAML_EMITTER_HPP_
#define PULMOTOR_YAML_EMITTER_HPP_

#include "stream.hpp"
#include "util.hpp"
#include <iostream>
#include <string_view>
#include <cinttypes>

using namespace std::literals;

namespace pulmotor
{

char spacer[8] = { 32,32,32,32, 32,32,32,32 };

namespace yaml
{

// values must not clash with writer::internal
enum fmt_flags : unsigned {
    block_literal = 0x01, // keep newlines when reading
    block_fold	  = 0x02, // fold newlines when reading

    eol_clip	  = 0x04, // single newline at the end of the block
    eol_strip	  = 0x08, // no newlines at the end
    eol_keep	  = 0x10, // preserve all newlines (including the ones after the block)
                          // if none is specified, it is computed automatically based on value
    single_quoted = 0x20,
    double_quoted = 0x40,
    flow		  = 0x80,
};

struct writer
{
    // key: &anchor !tag
    sink& m_sink;
    std::error_code m_ec;
    std::vector<unsigned> m_state_stack;
    unsigned m_state = 0;
    size_t m_prefix = 0;

    // std::string m_alias, m_anchor, m_tag;

    writer(sink& sink) : m_sink(sink)
    {}

    struct internal { enum : unsigned {
        key_written	   = 0x100,
        value_written  = 0x200,
        tag_written	   = 0x400,
        anchor_written = 0x800,
        property_written = tag_written | anchor_written,
        key_value_written = key_written | value_written,

        in_sequence	= 0x1000,
        in_mapping	= 0x2000,
        in_block	= 0x4000,
        in_container = in_sequence|in_mapping,

        eol_mask	  = fmt_flags::eol_clip|fmt_flags::eol_strip|fmt_flags::eol_keep,

        indent_mask	= 0x00f0'0000, indent_shift = 20,
        wrap_mask	= 0xff00'0000u, wrap_shift = 24
    }; };

    constexpr static fmt_flags indent(unsigned n) { assert(n < 10); return (fmt_flags)(n << internal::indent_shift); }
    constexpr static fmt_flags wrap(unsigned n) { assert(n < 255); return (fmt_flags)(n << internal::wrap_shift); }

    template<size_t N> void out(char const (&s)[N]) { m_sink.write(s, N-1, m_ec); }
    void out(char const* s, size_t len) { m_sink.write(s, len, m_ec); }
    void out(std::string_view s) { m_sink.write(s.data(), s.size(), m_ec); }

    // outputs indented text, wrapped at wrap (if non-null)
    // inserts newlines only when wrapping (no new line at the end)
    // when quoted, we add quote and the front and at the end, then replace EOL with NL and add backslash when wrapping
    void out_indented(std::string_view s, size_t indent, size_t wrap, bool quoted) {
        if (!wrap) wrap = s.size();
        if (quoted) out("\"");
        for(size_t i=0; i<s.size(); ) {
            out_prefix(indent);
            size_t left = s.size() - i;
            size_t write = left > wrap ? wrap : left;
            std::string_view sw = s.substr(i, write);
            assert (sw.size() > 0);
            // when quoted, \n -> '\n'; wrap with backslash
            if (quoted) {
                    size_t nl = sw.find_first_of('\n');
            } else {
                if (!std::isspace(sw.back())) {
                    size_t ws_pos = sw.find_last_of(" \n");
                    if (ws_pos != std::string_view::npos && ws_pos > 0) {
                        sw.remove_suffix(sw.size() - ws_pos - 1);
                    }
                }
            }

            size_t nl = s.substr(i, write).find_first_of('\n');
            if (nl != std::string_view::npos)
                write = nl + 1;
            out(s.substr(i, write));
            if ((i += write) != s.size() && (nl == std::string_view::npos)) out("\n");
        }
        if (quoted) out("\"");
    }

    void out_prefix() { out_prefix(m_prefix); }
    void out_prefix(size_t pref) {
        for (size_t left = pref; left > 0; ) {
            size_t w = left > sizeof spacer ? sizeof spacer : left;
            out(spacer, w);
            if (m_ec) return;
            left -= w;
        }
    }

    void prepare_node() {
        if (m_state & flow) {
            if (m_state & (internal::key_written|internal::property_written) )
                out(" ");
            else if (m_state & internal::value_written)
                out(", ");
            // else
            // 	out(" ");
        } else {
            if (m_state & internal::in_sequence) {
                if ( !(m_state & internal::property_written) ) {
                    out_prefix();
                    out("-");
                }
                out(" ");
            } else if (m_state & (internal::in_mapping|internal::property_written)) {
                if ( m_state & (internal::property_written|internal::key_written) )
                    out(" ");
            }
        }
    }

    void key_value(std::string_view a, std::string_view b) {
        key(a);
        value(b);
    }

    bool is_inmapping() const { return m_state & internal::in_mapping; }
    bool is_insequence() const { return m_state & internal::in_mapping; }
    bool is_incontainer() const { return m_state & internal::in_container; }

    void key(std::string_view a) {

        assert(m_state & internal::in_mapping);
        prepare_node();

        out(a.data(), a.size());
        out(":");
        m_state = m_state & ~internal::value_written & ~internal::property_written | internal::key_written;
    }

    void value(std::string_view a, unsigned flags = 0) {

        prepare_node();

        size_t indent = 1;
        bool add_eol = true;
        if (flags & (fmt_flags::block_literal|fmt_flags::block_fold)) {

            bool ends_with_eol = !a.empty() && a.back() == '\n';
            add_eol = !ends_with_eol;

            // we add eol only when there was none at the end of the supplied value.

            if (flags & fmt_flags::block_literal)
                out("|");
            else if (flags & fmt_flags::block_fold)
                out(">");

            if ( !(flags&internal::eol_mask) ) {

                // automatic eol flag detection
                if (!ends_with_eol) {
                    if (!a.empty()) out("-");
                } else {
                    if (a.size() > 1 && a[a.size()-2] == '\n')
                        out("+");
                }

            } else if (flags & fmt_flags::eol_strip)
                out("-");
            else if(flags & fmt_flags::eol_keep)
                out("+");

            if (flags & internal::indent_mask) {
                indent = (flags & internal::indent_mask) >> internal::indent_shift;
                char shift_digit = '0' + indent;
                out(std::string_view(&shift_digit, 1));
            }

            out("\n");

            size_t wrap = (flags & internal::wrap_mask) ? 0 : ((flags & internal::wrap_mask) >> internal::wrap_shift);
            out_indented(a, m_prefix + indent, wrap, false);

        } else {
            out(a);
        }

        m_state = m_state & ~internal::key_written & ~internal::property_written | internal::value_written;

        if ( !(m_state & flow) && add_eol )
            out("\n");
    }

    void sequence(unsigned flags = 0) {
        assert( !(m_state & flow) || ((m_state & flow) && (flags & flow))  );

        if ( (m_state & internal::in_container) && !(m_state & flow) && !(flags & flow))
            out("\n");

        if ( (m_state & flow) ) {
            if (m_state & (internal::key_written|internal::property_written))
                out(" ");
            else if (m_state & internal::value_written)
                out(", ");
        }

        if ( !(m_state & flow) && (m_state & internal::in_container))
            ++m_prefix;

        m_state_stack.push_back(m_state);
        m_state = internal::in_sequence | flags;

        if ( (m_state & flow) ) {
            out("[");
        }
    }

    void mapping(unsigned flags = 0) {
        assert( !(m_state & flow) || ((m_state & flow) && (flags & flow))  );

        if ( (m_state & internal::in_container) && !(m_state & flow) && !(flags & flow) )
            out("\n");

        if ( (m_state & flow) ) {
            if (m_state & internal::key_written)
                out(" ");
            else if (m_state & internal::value_written)
                out(", ");
        }

        if ( !(m_state & flow) && (m_state & internal::in_container))
            ++m_prefix;

        m_state_stack.push_back(m_state);
        m_state = internal::in_mapping | flags;

        if ( (m_state & flow) )
            out("{");
    }

    void end_container() {
        assert(!m_state_stack.empty());
        bool was_flow = m_state & flow;
        if (m_state & flow) {
            if (m_state & internal::in_mapping)
                out("}");
            if (m_state & internal::in_sequence)
                out("]");
        }

        m_state = m_state_stack.back();
        m_state_stack.pop_back();

        if ( !(m_state & flow) && (m_state & internal::in_container)) {
            assert(m_prefix >= 0);
            --m_prefix;
        }

        if ( was_flow && !(m_state & flow) )
            out("\n");
        else
            m_state = m_state & ~internal::key_written & ~internal::property_written | internal::value_written;
    }

    void tag(std::string_view s) {
        // assert( !(m_state & value_written) );
        // assert( !(m_state & key_written) );
        assert( !(m_state & internal::tag_written) );

        prepare_node();

        out("!");
        out(s);

        m_state |= internal::tag_written;
    }

    void anchor(std::string_view s) {
        // assert( !(m_state & value_written) );
        // assert( !(m_state & key_written) );
        assert( !(m_state & internal::anchor_written) );

        prepare_node();

        out("&");
        out(s);

        m_state |= internal::anchor_written;
    }

    void new_stream() {
        out("---\n");
    }

protected:
    void sprintf_value(std::string_view a, unsigned flags = 0) {
    }

};

struct writer_ostream : writer
{
    sink_ostream m_os_sink;

    writer_ostream(std::ostream& s)
    :   writer(m_os_sink)
    ,	m_os_sink(s)
    {}
};

template<class T>
struct printf_spec;

template<> struct printf_spec< std::int8_t> { using cast_to = std::int16_t; static constexpr char const* fmt = "%" PRId16; };
template<> struct printf_spec<std::int16_t> { using cast_to = std::int16_t; static constexpr char const* fmt = "%" PRId16; };
template<> struct printf_spec<std::int32_t> { using cast_to = std::int32_t; static constexpr char const* fmt = "%" PRId32; };
template<> struct printf_spec<std::int64_t> { using cast_to = std::int64_t; static constexpr char const* fmt = "%" PRId64; };

template<> struct printf_spec< std::uint8_t> { using cast_to = std::uint16_t; static constexpr char const* fmt = "%" PRIu16; };
template<> struct printf_spec<std::uint16_t> { using cast_to = std::uint16_t; static constexpr char const* fmt = "%" PRIu16; };
template<> struct printf_spec<std::uint32_t> { using cast_to = std::uint32_t; static constexpr char const* fmt = "%" PRIu32; };
template<> struct printf_spec<std::uint64_t> { using cast_to = std::uint64_t; static constexpr char const* fmt = "%" PRIu64; };

//template<class Writer>
//struct formatter : Writer
struct encoder
{
    template<class T>
    std::enable_if_t< std::is_integral_v<T>, void>
    static format_key(writer& w, T a) {
        char buff[32];
        size_t l = snprintf(buff, sizeof buff, printf_spec<T>::fmt, a);
        w.key(std::string_view(buff, l));
    }

    static void format_key(writer& w, std::string const& a) { w.key(a); }
    static void format_key(writer& w, std::string_view a) { w.key(a); }
    template<size_t N>
    static void format_key(writer& w, char const (&a)[N]) { w.key(std::string_view(a, N)); }

    template<class T>
    static void format_value(writer& w, T a, std::enable_if_t< std::is_floating_point_v<T>, unsigned> flags = 0) {
        if constexpr (std::is_same<T, float>::value)
            sprintf_float(w, flags, a);
        else
            sprintf_double(w, flags, a);
    }

    static void format_value(writer& w, std::string_view a) { w.value(a); }

    static void sprintf_float(writer& w, unsigned flags, float a) {
        char buff[32];
        size_t l = snprintf(buff, sizeof buff, "%f", a); // TODO: use ryu
        w.value(std::string_view(buff, l), flags);
    }

    static void sprintf_float(writer& w, unsigned flags, double a) {
        char buff[32];
        size_t l = snprintf(buff, sizeof buff, "%f", a); // TODO: use ryu
        w.value(std::string_view(buff, l), flags);
    }

    template<class T>
    static void format_value(writer& w, T a, std::enable_if_t< std::is_integral_v<T>, unsigned> flags = 0) {
        sprintf_value(w, flags, printf_spec<T>::fmt, a);
    }

    static void sprintf_value(writer& w, unsigned flags, char const* spec, ...) {
        va_list vl;
        va_start(vl, spec);
        char buff[32];
        size_t l = vsnprintf(buff, sizeof buff, spec, vl);
        w.value(std::string_view(buff, l), flags);
        va_end(vl);
    }

    static void encode_value(writer& w, char const* data, size_t size, size_t wrap = 0) {
        std::string buff;
        buff.resize(util::base64_encode_length(size, util::base64_options::pad));
        util::base64_encode(data, size, buff.data(), util::base64_options::pad);

        w.tag("!binary");
        w.value(buff, writer::wrap(wrap));
    }
};

} // yaml

} // pulmotor

#endif // PULMOTOR_YAML_EMITTER_HPP_
