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

// struct break_iterator
// {
//     char const* m_ptr;
// };

// struct escape_scanner
// {
//     char const* m_ptr;
//     size_t m_size;

//     bool advance(ssize_t dist)
// };

char spacer[8] = { 32,32,32,32, 32,32,32,32 };

namespace yaml
{

// values must not clash with writer::internal
enum fmt_flags : unsigned {
    fmt_none      = 0x00,
    // fmt_mask      = 0xff,

    // block notation, quotes are parsed as are (thus quoted makes no sense in block notation)
    block_literal = 0x01, // block notation, literal style (|). keep newlines
    block_folded  = 0x02, // block notation, folded style (>). folds newlines

    // if no EOL processing is specified, it is computed automatically based on value
    eol_clip	  = 0x04, // single newline at the end of the block
    eol_strip	  = 0x08, // no newlines at the end
    eol_keep	  = 0x10, // preserve all newlines (including the ones after the block)

    eol_mask      = eol_clip|eol_strip|eol_keep,

    single_quoted = 0x20,
    double_quoted = 0x40,

    flow		  = 0x80,

    key_indicator = 0x100,
    keep_entry    = 0x200,
    is_text       = 0x400, // prefix processing is performed
    // fmt_unfinished= 0x400,
    wrap_space    = 0x1000, // wrap only on space. when long - don't wrap
    force_break   = 0x2000,
    // no_escape    = 0x4000,

    wrap_mask     = 0xffff0000,
    wrap_shift    = 16
};
constexpr fmt_flags operator|(fmt_flags a, fmt_flags b) { return (fmt_flags)((unsigned)a | (unsigned)b); }

// sequence:
// - VALUE
// flow-sequence: [ value , value ]
//
// mapping:
// ? key
// : value
// flow-mapping: { key : value, key : value }
// # comment
// & anchor
// * alias
// ! tag
// | literal block scalar
// > folded block scalar
// ' single quoted flow scalar
// " double quoted flow scalar
// % directive
// @ and ` reserved for future
// prefix
//  seq::
//      block:: indicator value
//      flow:: [ value , ]
//  map::
//      block:: indicator? key: value
//      flow:: { key : value }


class writer1
{
    enum ctx : u8 {
        ctx_block,
        ctx_flow
    };

    // enum {
    //     st_literal,
    //     st_folded,
    //     st_plain,
    //     st_single_quote,
    //     st_double_quote
    // };

    enum knd : u8 {
        knd_scalar = 0,
        knd_sequence = 1,
        knd_mapping = 2
    };
    struct state
    {
        u8 wr_flags = 0;
        u8 fmt = 0;
        u8 prefix = 0;
        ctx context:1 = ctx_block;
        knd parent:2 = knd_scalar;
    };

    state m_state;
    std::ostream& m_outs;
    std::vector<state> m_sstack;
    // size_t m_prefix = 0;

    template<size_t N> void out(char const v[N]) { out(std::string_view(v, N)); }
    void out(std::string_view v) { m_outs << v; }
    void out(char c) { m_outs << c; } // out(std::string_view(&c, 1)); }
    void out_prefix(size_t distance) {
        if (distance) {
            char prev = m_outs.fill(' ');
            m_outs.width(distance);
            m_outs << "";
            m_outs.fill(prev);
        }
        // for (size_t left = distance; left > 0; ) {
        //     size_t w = left > sizeof spacer ? sizeof spacer : left;
        //     m_outs.write(spacer, w);
        //     left -= w;
        // }
    }

    void out_raw(std::string_view s)
    {
        out(s);
    }

    // wrap escaped or on space (multiple space have to be escaped or trail), escape eols or change to double (only the first)
    void out_quoted(std::string_view s, size_t indent, char quote, fmt_flags flags, bool escape_eol, size_t wrap)
    {
        assert (quote != 0);

        char const* esc_symbols = [](char q) { return q == '"' ? "\n\"" : "\n'"; } (quote);
        auto remap_esc_to_char = [](char q) { return q == '\n' ? 'n' : q == '\r' ? 'r' : q; };

        for(size_t i=0; i<s.size(); ) {

            if (indent)
                out_prefix(indent);

            size_t left = s.size() - i;
            size_t write = left > wrap ? wrap : left;

            std::string_view l = s.substr(i);

            if (flags & wrap_space) {
                assert ((flags & block_literal) == false);
                size_t sp_fwd = s.find_first_of(" \n", i + wrap);
                l = s.substr(i, sp_fwd); // npos fpr length means the whole string
                if (wrap && l.size() > wrap) {
                    // if it's longer than wrap, search backwards for a space and truncate until there. if none found, use the whole string
                    size_t sp_back = l.find_last_of(" \n");
                    if (sp_back != std::string_view::npos) {
                        l = s.substr(i, sp_back);
                    }
                }
            }

            out(l);

            if (flags & wrap_space) {
                out('\n');
                assert ((flags & block_literal) == false);
                // if we wrapped on EOL we want to double the EOL as two EOL is parsed as one.
                if (!l.empty() && l.back() == '\n')
                    out('\n');
            }
        }
    }

    // MODES
    //  - WRAP: no wrap / wrap strict on limit (with escape) / wrap on space
    //  - WHEN WRAPPING: nothing (insert LINEBREAK) / add escape (and insert LINEBREAK)
    //  - EOL PROCESSING: nothing / replace with \n

    // indents, formats (replace newlines with \n when quotes) and wraps (if wrap is non-null)
    // inserts newlines only when wrapping (no new line at the end)
    // when quoted, we add quote and the front and at the end, then replace EOL with NL and add backslash when wrapping
    void out_block_notation(std::string_view s, size_t indent, fmt_flags flags, size_t wrap = 0) {

        if (!wrap) wrap = s.size();

        bool is_bock = flags & (block_folded|block_literal);

        for(size_t i=0; i<s.size(); ) {

            if (indent)
                out_prefix(indent);

            // eol_clip     single newline at the end of the block
            // eol_strip    no newlines at the end
            // eol_keep     preserve all newlines (including the ones after the block)
            size_t left = s.size() - i;
            size_t write = left > wrap ? wrap : left;

            std::string_view l = s.substr(i);

            if (flags & wrap_space) {
                assert ((flags & block_literal) == false);
                size_t sp_fwd = s.find_first_of(" \n", i + wrap);
                l = s.substr(i, sp_fwd); // npos means the whole string
                if (wrap && l.size() > wrap) {
                    // if it's longer than wrap, search backwards for a space and truncate until there. if none, use the whole string
                    size_t sp_back = l.find_last_of(" \n");
                    if (sp_back != std::string_view::npos) {
                        l = s.substr(i, sp_back);
                    }
                }
            }

            out(l);

            if (flags & wrap_space) {
                out('\n');
                assert ((flags & block_literal) == false);
                // if we wrapped on EOL we want to double the EOL as two EOL is parsed as one.
                if (!l.empty() && l.back() == '\n')
                    out('\n');
            }
        }
    }

    static bool is_control(unsigned c) { return c == 0x1f; }
    static bool is_special(unsigned c) { return c == 0x1f; }
    static bool is_linebreak(unsigned c) { return c == 0x0a; }
    static bool is_wordbreak(unsigned c) { return c == ' '; }
    static bool is_blank(unsigned c) { return c == ' ' || c == '\t'; }

    static bool is_nbjson(unsigned c) { return c == 0x09 || c >= 0x20; }
    static bool is_escape(unsigned c) { return c == 0 || (c >= 0x07 && c <= 0x0d); }
// - "\" \a \b \e \f"
// - "\n \r \t \v \0"
// - "\  \_ \N \L \P
    static unsigned transform_for_escape(unsigned cp) {
        switch(cp) {
            case 0x0000: return '0';
            case 0x0007: return 'a';
            case 0x0008: return 'b';
            case 0x0009: return 't';
            case 0x000a: return 'n';
            case 0x000b: return 'v';
            case 0x000c: return 'f';
            case 0x000d: return 'r';
            case 0x001b: return 'e';
            case 0x0020: return ' ';
            case 0x0022: return '"';
            case 0x002f: return '/';
            case 0x005c: return '\\';
            case 0x0085: return 'N';
            case 0x00a0: return '_';
            case 0x2028: return 'L';
            case 0x2029: return 'P';
            default: return cp;
        }
    }

    void advance_blanks(char const*& s, char const* e) {
        char const* p = s;
        while(p != e) {
            unsigned cp = utf8::next_unchecked(p);
            if (!is_blank(cp)) break;
            s = p;
        }
    }

    bool out_text_block(std::string_view s, fmt_flags flags, unsigned indent, unsigned wrap = 0)
    {
        char const* p = s.data();
        char const* e = s.end();
        char const* start = p;
        size_t w = 0;

        bool indented = true;
        auto do_flush = [e, &start, &indented, indent, this]
            (char const* span_end, char const* new_start) {
            assert(span_end <= e);
            if (!indented) {
                out_prefix(indent);
                indented = true;
            }
            out(std::string_view(start, span_end - start));
            start = new_start;
        };
        auto do_wrap = [this, &w, &indented]() {
            out_linebreak();
            indented = false;
            w = 0;
        };

        char q;
        if (flags & (fmt_flags::double_quoted|fmt_flags::single_quoted)) {
            q = flag_to_quote(flags);
            out(q);
        }

        bool seq_eol = false;
        bool islb = false;
        for (; p < e; )
        {
            char const* prev = p;
            unsigned cp = utf8::next_unchecked(p);
            islb = is_linebreak(cp);

            bool flush = false;
            if (flags & (fmt_flags::double_quoted|fmt_flags::single_quoted)) {
                if (flags & fmt_flags::single_quoted) {
                    if (islb) PULMOTOR_THROW_FMT_ERROR("Linebreaks are not supported in single quoted strings. The offending string was '%.*s'", s.size(), s.data());
                    // can break only on spaces
                    if (wrap && w >= wrap && is_wordbreak(cp)) {
                        advance_blanks(p, e);
                        do_flush(p, p);
                        do_wrap();
                        continue;
                    }
                    if (cp == '\'')
                        do_flush(p, p), out('\''), ++w;
                } else if (flags & fmt_flags::double_quoted) {
                    // can break anywhere escaping EOL
                    if (wrap && w >= wrap) {
                        // if (is_wordbreak(cp)) { // if we're to break on blank, must keep it at the end of line otherwise it's stripped
                        advance_blanks(p, e);
                        do_flush(p, p);
                        if (p == e) break;
                        out('\\');
                        do_wrap();
                    }
                    if (cp == '\"' || cp == '\\')
                        do_flush(p, p), w++, out('\\');
                    else if (is_escape(cp)) {
                        do_flush(prev, p); // do not write the escape character as it is
                        unsigned x = transform_for_escape(cp);
                        out('\\'), ++w;
                        out(x), ++w;
                    }
                }
            } else {
                if (islb) {
                    do_flush(prev, p);
                    do_wrap();
                } else if (wrap && w > wrap && (is_wordbreak(cp) || (flags&fmt_flags::force_break)) ) {
                    advance_blanks(p, e);
                    do_flush(p, p);
                    do_wrap();
                }
            }
        }

        if (start != e)
            do_flush(p, p);

        if (flags & (fmt_flags::double_quoted|fmt_flags::single_quoted))
            out(q);

        return islb;
    }

    void end_block_entry() {
        if (m_state.context == ctx_block)
            m_state.wr_flags &= ~(wr_some_value|wr_start_indicator);
        out_linebreak();
    }
    void out_linebreak() {
        out("\n");
    }

    void out_space() { out(" "); }

    static bool is_eol(unsigned v) { return v == '\n'; }
    constexpr static char flag_to_quote(fmt_flags f) { return (f & single_quoted) ? '\'' : (f & double_quoted) ? '"' : 0; }

    enum { // written
        wr_key = 0x01,
        wr_value = 0x02,
        wr_tag = 0x04,
        wr_anchor = 0x08,
        wr_property = wr_tag | wr_anchor,
        wr_start_indicator = 0x80,
        wr_flow_content = 0x40, // when in flow style and some content is written in a collection
        wr_alias = 0x20,
        wr_some_content = wr_property|wr_key|wr_value|wr_alias,
        wr_some_value = wr_property|wr_value|wr_alias,
    };

    void write_block_style_indicator(fmt_flags fflags)
    {
        assert((m_state.wr_flags & wr_value) == 0);

        if (fflags & block_folded) {
            out('>');
        } else if (fflags & block_literal)
            out('|');

        if (fflags & eol_strip)
            out('-');
        else if (fflags & eol_keep)
            out('+');
    }

    bool write_start_indicator(fmt_flags flags) // [ or { or - or ?
    {
        assert( (m_state.wr_flags & wr_start_indicator) == 0);

        bool written = false;

        switch (m_state.parent) {
            case knd_sequence:
                if (m_state.context == ctx_block)
                    out('-'), written = true;
                else if (m_state.context == ctx_flow)
                    out('['), written = true;
            break;

            case knd_mapping:
                if (m_state.context == ctx_block && (flags & fmt_flags::key_indicator))
                    out('?'), written = true;
                else if (m_state.context == ctx_flow) {
                    assert( (m_state.wr_flags & wr_key) == 0);
                    out('{'), written = true;
                }
            break;

            case knd_scalar: // nothing here, folding style is emitted in another function if any specified
            break;
        }
        m_state.wr_flags |= wr_start_indicator;
        return written;
    }

    // basically this is a comma separating entries in flow formatting
    void write_entry_end_indicator() // [ or { or - or ?
    {
        // assert ( (m_state.wr_flags & wr_entry_indicator) == 0);
        // assert ( (m_state.wr_flags & wr_flow_content) );
        assert ( m_state.context == ctx_flow);

        out(',');
        m_state.wr_flags &= ~(wr_some_content|wr_anchor);
    }

    fmt_flags recompute_eol_flags(std::string_view s, fmt_flags flags)
    {
        // EOL{1} -> nothing
        // [^EOL] -> strip
        // EOL+ -> keep
        char const* last = s.end();
        unsigned n = 0;
        do {
            utf8::backtrack_codepoint_start(last, s.begin());
            if (last < s.begin()) break;

            unsigned cp = utf8::decode_unchecked(last);
            if (is_eol(cp)) {
                if (flags & fmt_flags::eol_keep) {
                    if (++n > 1) break;
                } else {
                    n++;
                    break;
                }
            }
            break;
        } while(1);

        if (n > 0)
            if (n > 1)
                flags = flags|fmt_flags::eol_keep;
            else
                flags = flags|fmt_flags::eol_clip;
        else
            flags = flags|fmt_flags::eol_strip;

        return flags;
    }

public:

// prefix
//  seq::
//      block:: indicator alias? properties* value
//      flow:: [ value , ]
//  map::
//      block:: indicator? key: alias? properties* value
//      flow:: { key : value }
//
// properties ::= !tag | &anchor

    static fmt_flags wrap(unsigned n) { assert(n < 65535); return (fmt_flags)(n << fmt_flags::wrap_shift); }

    writer1(std::ostream& os)
    :   m_outs(os)
    {
    }

    void sequence(fmt_flags flags = fmt_flags::fmt_none)
    {
        // assert ( (flags & fmt_flags::flow) == 0 && (m_state.fmt & fmt_flags::flow) == 0 && "flow to non flow is disallowed" );
        bool is_scalar = m_state.parent == knd_scalar;

        m_sstack.push_back(m_state);
        m_state.parent = knd_sequence;
        if (flags & fmt_flags::flow) {
            if (!is_scalar) {
                if (m_state.wr_flags & (wr_some_value))
                    write_entry_end_indicator();
                out_space();
            }
            out("[");
            m_state.context = ctx_flow;
            m_state.wr_flags = wr_start_indicator;
        } else {
            // no point is writing item indicator here, as it will have to written in value writing on non-first array item
            // thus to concentrate logic in one place, write indicators there only.
            // out("-");
            if (!is_scalar)
                prepare_block_entry(flags);

            if (m_state.context == ctx_flow)
                PULMOTOR_THROW_FMT_ERROR("Can not go from flow to block context");
            m_state.context = ctx_block;
            m_state.wr_flags = 0;
            if (!is_scalar)
                out_linebreak(), m_state.prefix++;
            // prepare_block_entry(flags);
        }
    }

    void mapping(fmt_flags flags = fmt_flags::fmt_none)
    {
        // assert ( (flags & fmt_flags::flow) == 0 && (m_state.fmt & fmt_flags::flow) == 0 && "flow to non flow is disallowed" );
        bool is_scalar = m_state.parent == knd_scalar;
        m_sstack.push_back(m_state);

        if ((flags & fmt_flags::flow) || m_state.context == ctx_flow) {
            if (!is_scalar) {
                if (m_state.wr_flags & (wr_some_value))
                    write_entry_end_indicator();
                out_space();
            }
            out("{");
            m_state.context = ctx_flow;
            m_state.wr_flags = wr_start_indicator;
        } else {
            if (!is_scalar)
                prepare_block_entry(flags);
            m_state.wr_flags = 0;
            m_state.context = ctx_block;
            if (!is_scalar) {
                out_linebreak(), m_state.prefix++;
            }
        }

        m_state.parent = knd_mapping;
    }

    void end_collection()
    {
        bool is_flow = m_state.context == ctx_flow;
        if (is_flow) {
            if (m_state.parent == knd_sequence)
                out_space(), out(']');
            else if (m_state.parent == knd_mapping)
                out_space(), out('}');
        }

        assert(!m_sstack.empty());
        m_state = m_sstack.back();
        m_sstack.pop_back();

        // we've written value for this entry, mark it as so
        m_state.wr_flags |= wr_value;

        // if (m_state.context == ctx_block && (m_state.wr_flags & wr_some_content))
        //     end_block_entry();
    }

    void prepare_block_entry(fmt_flags flags)
    {
        assert(m_state.parent != knd_scalar);
        if ( (m_state.wr_flags & wr_start_indicator) == 0) {
            out_prefix(m_state.prefix);
            write_start_indicator(flags);
        }
    }

    static constexpr unsigned add_indent = 1;

    void force_start_entry()
    {
        if (m_state.context == ctx_flow) {
            if (m_state.wr_flags & (wr_tag))
                out_space();
            write_entry_end_indicator();
        } else
            out_linebreak();

        m_state.wr_flags &= ~wr_some_content;
    }

    void value(std::string_view v, fmt_flags flags = fmt_none)
    {
        switch(m_state.context)
        {
            case ctx_flow:
                assert(m_state.wr_flags & wr_start_indicator);
                assert((m_state.wr_flags & wr_alias) == 0 && "entry already has an alias, can't write value");

                if (m_state.wr_flags & (wr_value|wr_alias))
                    write_entry_end_indicator();

                m_state.wr_flags |= wr_value;
                out_space();
                break;
            case ctx_block:
                if (m_state.parent != knd_scalar) {
                    prepare_block_entry(flags);
                    out_space();
                }

                // TODO: compute EOL handling only for blocks?
                if ((flags & (block_folded|block_literal)) && (flags & fmt_flags::eol_mask) == 0) {
                    flags = recompute_eol_flags(v, flags);
                }

                // write any literal/folder indicators if we need to
                if (flags & (block_folded|block_literal)) {
                    write_block_style_indicator(flags);
                    end_block_entry();
                    out_prefix(m_state.prefix + add_indent);
                }
                break;
        }

        bool text_ends_with_linebreak = false;
        if (flags & (fmt_flags::is_text|fmt_flags::single_quoted|fmt_flags::double_quoted)) {
            size_t wrap = (flags & fmt_flags::wrap_mask) >> fmt_flags::wrap_shift;
            text_ends_with_linebreak = out_text_block(v, flags, m_state.prefix + add_indent, wrap);
        } else {
            out_raw(v);
        }

        if (!text_ends_with_linebreak && m_state.context != ctx_flow)
            end_block_entry();
    }

    void key(std::string_view k, fmt_flags flags = fmt_none)
    {
        assert(m_state.parent == knd_mapping);

        if (m_state.context == ctx_flow) {
            assert((m_state.wr_flags & wr_start_indicator) != 0);
            if (m_state.wr_flags & (wr_value|wr_alias|wr_tag))
                write_entry_end_indicator();
            out_space();
        } else {
            assert(m_state.context == ctx_block);
            if ( (m_state.wr_flags & wr_start_indicator) == 0) {
                out_prefix(m_state.prefix);
                if (write_start_indicator(flags))
                    out_space();
            }
        }

        out(k);
        out(':');

        m_state.wr_flags |= wr_key;
        m_state.wr_flags &= ~(wr_anchor|wr_tag); // now anchor or tag can be written again for this entry's value
    }

    void key_value(std::string_view a, std::string_view b)
    {
        key(a);
        value(b);
    }

    void tag(std::string_view v, fmt_flags flags = fmt_flags::fmt_none)
    {
        if (m_state.context == ctx_flow) {
            assert(m_state.wr_flags & wr_start_indicator);
            if (m_state.wr_flags & (wr_value|wr_alias) ||
                ((m_state.wr_flags & wr_tag) && (flags & keep_entry) == 0))
                write_entry_end_indicator();
            out_space();
        } else {
            if (m_state.parent != knd_scalar) {
                if ( (m_state.wr_flags & wr_start_indicator) == 0) {
                    out_prefix(m_state.prefix);
                    write_start_indicator(fmt_flags::fmt_none);
                }
                out_space();
            }
        }

        out(v);
        m_state.wr_flags |= wr_tag;
    }

    void prepare_flow_entry(fmt_flags flags) {
        if (m_state.wr_flags & wr_value) {
            write_entry_end_indicator();
        } else
            m_state.wr_flags |= wr_value;
    }

    void anchor(std::string_view v, fmt_flags flags = fmt_none)
    {
        if (m_state.context == ctx_flow) {
            assert(m_state.wr_flags & wr_start_indicator);
            if (m_state.wr_flags & (wr_value|wr_alias) ||
                ((m_state.wr_flags & wr_anchor) && (flags & keep_entry) == 0) )
                write_entry_end_indicator();
        } else {
            assert(m_state.context == ctx_block);
            if (m_state.wr_flags & wr_value)
                PULMOTOR_THROW_FMT_ERROR("anchor shall appear before value");
            prepare_block_entry(flags);
        }
        out_space();

        out('&');
        out(v);

        m_state.wr_flags |= wr_anchor;
    }

    void alias(std::string_view v, fmt_flags flags = fmt_none)
    {
        if (m_state.wr_flags & wr_property)
            PULMOTOR_THROW_FMT_ERROR("alias entry shall not have any properties");

        if (m_state.context == ctx_flow) {
            assert(m_state.wr_flags & wr_start_indicator);
            prepare_flow_entry(flags);
        } else {
            prepare_block_entry(flags);
        }
        out_space();

        out('*');
        out(v);

        m_state.wr_flags |= wr_alias;
    }

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
        key_written	     = 0x100,
        value_written    = 0x200,
        tag_written	     = 0x400,
        anchor_written   = 0x800,
        entry_written    = 0x4000,
        property_written = tag_written | anchor_written,
        key_value_written = key_written | value_written,

        in_sequence	  = 0x1000,
        in_mapping	  = 0x2000,
        in_block	  = 0x4000,
        in_collection = in_sequence|in_mapping,

        eol_mask	  = fmt_flags::eol_clip|fmt_flags::eol_strip|fmt_flags::eol_keep,

        indent_mask	= 0x00f0'0000, indent_shift = 20,
        wrap_mask	= 0xff00'0000u, wrap_shift = 24
    }; };

    constexpr static fmt_flags indent(unsigned n) { assert(n < 10); return (fmt_flags)(n << internal::indent_shift); }
    constexpr static fmt_flags wrap(unsigned n) { assert(n < 255); return (fmt_flags)(n << internal::wrap_shift); }
    constexpr static char flag_to_quote(fmt_flags f) { return (f & single_quoted) ? '\'' : (f & double_quoted) ? '"' : 0; }

    template<size_t N> void out(char const (&s)[N]) { m_sink.write(s, N-1, m_ec); }
    void out(char const* s, size_t len) { m_sink.write(s, len, m_ec); }
    void out(std::string_view s) { m_sink.write(s.data(), s.size(), m_ec); }
    void out(char c) { m_sink.write(&c, 1, m_ec); }

    // outputs indented text, wrapped at wrap (if non-null)
    // inserts newlines only when wrapping (no new line at the end)
    // when quoted, we add quote and the front and at the end, then replace EOL with NL and add backslash when wrapping
    void out_indented(std::string_view s, size_t indent, size_t wrap, char quote = 0) {
        if (!wrap) wrap = s.size();
        if (quote) out(quote);
        for(size_t i=0; i<s.size(); ) {
            out_prefix(indent);
            size_t left = s.size() - i;
            size_t write = left > wrap ? wrap : left;
            std::string_view sw = s.substr(i, write);
            assert (sw.size() > 0);
            // when quoted, \n -> '\n'; wrap with backslash
            if (quote) {
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
        if (quote) out(quote);
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
            } else if (m_state & (internal::in_mapping)) {
                if ( !(m_state & (internal::property_written|internal::key_written)) )
                    out_prefix();
                else if ( m_state & internal::key_written )
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
    bool is_incollection() const { return m_state & internal::in_collection; }

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
        if (flags & (fmt_flags::block_literal|fmt_flags::block_folded)) {

            bool ends_with_eol = !a.empty() && a.back() == '\n';
            add_eol = !ends_with_eol;

            // we add eol only when there was none at the end of the supplied value.

            if (flags & fmt_flags::block_literal)
                out("|");
            else if (flags & fmt_flags::block_folded)
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
            out_indented(a, m_prefix + indent, wrap, flag_to_quote((fmt_flags)flags));

        } else {
            if (flags & (single_quoted|double_quoted)) {
                char q = flag_to_quote((fmt_flags)flags);
                out(q);
                out(a);
                out(q);
            } else
                out(a);
        }

        m_state &= ~internal::key_written & ~internal::property_written;
        m_state |= internal::value_written;

        if ( !(m_state & flow) && add_eol )
            out("\n");
    }

    void sequence(unsigned flags = 0) {
        assert( !(m_state & flow) || ((m_state & flow) && (flags & flow))  );

        if ( (m_state & internal::in_collection) && !(m_state & flow) && !(flags & flow))
            out("\n");

        if ( (m_state & flow) ) {
            if (m_state & (internal::key_written|internal::property_written))
                out(" ");
            else if (m_state & internal::value_written)
                out(", ");
        }

        if ( !(m_state & flow) && (m_state & internal::in_collection))
            ++m_prefix;

        m_state_stack.push_back(m_state);
        m_state = internal::in_sequence | flags;

        if ( (m_state & flow) )
            out("[");
        else {
            out_prefix();
            out("-");
        }
    }

    void mapping(unsigned flags = 0) {
        assert( !(m_state & flow) || ((m_state & flow) && (flags & flow))  );

        if ( (m_state & internal::in_collection) && !(m_state & flow) && !(flags & flow) )
            out("\n");

        if ( (m_state & flow) ) {
            if (m_state & internal::key_written)
                out(" ");
            else if (m_state & internal::value_written)
                out(", ");
        }

        if ( !(m_state & flow) && (m_state & internal::in_collection))
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

        if ( !(m_state & flow) && (m_state & internal::in_collection)) {
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

template<> struct printf_spec< bool> { using cast_to = int; static constexpr char const* fmt = "%d"; };

template<> struct printf_spec< std::int8_t> { using cast_to = std::int16_t; static constexpr char const* fmt = "%" PRId16; };
template<> struct printf_spec<std::int16_t> { using cast_to = std::int16_t; static constexpr char const* fmt = "%" PRId16; };
template<> struct printf_spec<std::int32_t> { using cast_to = std::int32_t; static constexpr char const* fmt = "%" PRId32; };
template<> struct printf_spec<std::int64_t> { using cast_to = std::int64_t; static constexpr char const* fmt = "%" PRId64; };

template<> struct printf_spec< std::uint8_t> { using cast_to = std::uint16_t; static constexpr char const* fmt = "%" PRIu16; };
template<> struct printf_spec<std::uint16_t> { using cast_to = std::uint16_t; static constexpr char const* fmt = "%" PRIu16; };
template<> struct printf_spec<std::uint32_t> { using cast_to = std::uint32_t; static constexpr char const* fmt = "%" PRIu32; };
template<> struct printf_spec<std::uint64_t> { using cast_to = std::uint64_t; static constexpr char const* fmt = "%" PRIu64; };

struct encoder
{
    template<class T>
    std::enable_if_t< std::is_integral_v<T>, void>
    static format_key(writer1& w, T a) {
        char buff[32];
        size_t l = snprintf(buff, sizeof buff, printf_spec<T>::fmt, a);
        w.key(std::string_view(buff, l));
    }

    static void format_key(writer1& w, std::string const& a) { w.key(a); }
    static void format_key(writer1& w, std::string_view a) { w.key(a); }
    template<size_t N>
    static void format_key(writer1& w, char const (&a)[N]) { w.key(std::string_view(a, N)); }

    template<class T>
    static void format_value(writer1& w, T a, std::enable_if_t< std::is_floating_point_v<T>, fmt_flags> flags = 0) {
        if constexpr (std::is_same<T, float>::value)
            sprintf_float(w, flags, a);
        else
            sprintf_double(w, flags, a);
    }

    static void format_value(writer1& w, std::string_view a, fmt_flags flags = fmt_none) { w.value(a, flags); }

    static void sprintf_float(writer1& w, fmt_flags flags, float a) {
        char buff[32];
        size_t l = snprintf(buff, sizeof buff, "%f", a); // TODO: use ryu
        w.value(std::string_view(buff, l), flags);
    }

    static void sprintf_double(writer1& w, fmt_flags flags, double a) {
        char buff[32];
        size_t l = snprintf(buff, sizeof buff, "%f", a); // TODO: use ryu
        w.value(std::string_view(buff, l), flags);
    }

    template<class T>
    static void format_value(writer1& w, T a, std::enable_if_t< std::is_integral_v<T>, fmt_flags> flags = fmt_flags::fmt_none) {
        sprintf_value(w, flags, printf_spec<T>::fmt, a);
    }

    static void sprintf_value(writer1& w, fmt_flags flags, char const* spec, ...) {
        va_list vl;
        va_start(vl, spec);
        char buff[32];
        size_t l = vsnprintf(buff, sizeof buff, spec, vl);
        w.value(std::string_view(buff, l), flags);
        va_end(vl);
    }

    static void encode_value(writer1& w, char const* data, size_t size, size_t wrap = 0) {
        std::string buff;
        buff.resize(util::base64_encode_length(size, util::base64_options::pad));
        util::base64_encode(data, size, buff.data(), util::base64_options::pad);

        w.tag("!binary");
        w.value(buff, writer1::wrap(wrap));
    }
};

} // yaml

} // pulmotor

#endif // PULMOTOR_YAML_EMITTER_HPP_
