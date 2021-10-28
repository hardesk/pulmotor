#ifndef PULMOTOR_YAML_EMITTER_HPP_
#define PULMOTOR_YAML_EMITTER_HPP_

#include "stream.hpp"
#include "util.hpp"
#include <iostream>
#include <string_view>

using namespace std::literals;

namespace pulmotor
{

char spacer[8] = { 32,32,32,32, 32,32,32,32 };

namespace yaml
{

struct writer
{
    // key: &anchor !tag
    sink& m_sink;
    std::error_code m_ec;
    std::vector<unsigned> m_state_stack;
    unsigned m_state = 0;
    size_t m_prefix = 0;

    std::string m_alias, m_anchor, m_tag;

    writer(sink& sink) : m_sink(sink)
    {}

    enum flags : unsigned {
        block_literal = 0x100, // keep newlines when reading
        block_fold	  = 0x200, // fold newlines when reading

		// if none is specified, it is computed automatically based on value
		eol_clip	  = 0x0400, // single newline at the end of the block
		eol_strip	  = 0x0800, // no newlines at the end
		eol_keep	  = 0x1000, // preserve all newlines (including the ones after the block)

        single_quoted = 0x1'0000,
        double_quoted = 0x2'0000,
        flow		  = 0x8'0000,
    };

	struct internal { enum : unsigned {
		key_written	   = 0x01,
		value_written  = 0x02,
		tag_written	   = 0x04,
		anchor_written = 0x08,
		property_written = tag_written | anchor_written,
		key_value_written = key_written | value_written,

		in_sequence	= 0x10,
		in_mapping	= 0x20,
		in_block	= 0x40,
		in_container = in_sequence|in_mapping,

		eol_mask	  = eol_clip|eol_strip|eol_keep,

		indent_mask	= 0x00f0'0000, indent_shift = 20,
		wrap_mask	= 0xff00'0000u, wrap_shift = 24
	}; };

	constexpr static flags indent(unsigned n) { assert(n < 10); return (flags)(n << internal::indent_shift); }
	constexpr static flags wrap(unsigned n) { assert(n < 255); return (flags)(n << internal::wrap_shift); }

    template<size_t N> void out(char const (&s)[N]) { m_sink.write(s, N-1, m_ec); }
    void out(char const* s, size_t len) { m_sink.write(s, len, m_ec); }
    void out(std::string_view s) { m_sink.write(s.data(), s.size(), m_ec); }

	// outputs indented text, wrapped at wrap (if non-null)
	// inserts newlines only when wrapping (no new line at the end)
    void out_indented(std::string_view s, size_t indent, size_t wrap) {
		if (!wrap) wrap = s.size();
		for(size_t i=0; i<s.size(); ) {
			out_prefix(indent);
			size_t left = s.size() - i;
			size_t write = left > wrap ? wrap : left;
			size_t nl = s.substr(i, write).find_first_of('\n');
			if (nl != std::string_view::npos)
				write = nl + 1;
			out(s.substr(i, write));
			if ((i += write) != s.size() && (nl == std::string_view::npos)) out("\n");
		}
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
			} else if (m_state & internal::in_mapping) {
				if ( m_state & (internal::property_written|internal::key_written) )
					out(" ");
			}
		}
	}

    void key_value(std::string_view a, std::string_view b) {
		key(a);
		value(b);
    }

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
		if (flags & (block_literal|block_fold)) {

			bool ends_with_eol = !a.empty() && a.back() == '\n';
			add_eol = !ends_with_eol;

			// we add eol only when there was none at the end of the supplied value.

			if (flags & block_literal)
				out("|");
			else if (flags & block_fold)
				out(">");

			if ( !(flags&internal::eol_mask) ) {

				// automatic eol flag detection
				if (!ends_with_eol) {
					if (!a.empty()) out("-");
				} else {
					if (a.size() > 1 && a[a.size()-2] == '\n')
						out("+");
				}

			} else if (flags & eol_strip)
				out("-");
			else if(flags & eol_keep)
				out("+");

			if (flags & internal::indent_mask) {
				indent = (flags & internal::indent_mask) >> internal::indent_shift;
				char shift_digit = '0' + indent;
				out(std::string_view(&shift_digit, 1));
			}

			out("\n");

			size_t wrap = (flags & internal::wrap_mask) ? 0 : ((flags & internal::wrap_shift) >> internal::wrap_shift);
			out_indented(a, m_prefix + indent, wrap);

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
};

struct writer_ostream : writer
{
    // std::ostream& m_stream;
    sink_ostream m_os_sink;

    writer_ostream(std::ostream& s)
    :   m_os_sink(s)
    ,   writer(m_os_sink)
    {}
};

//template<class Writer>
//struct formatter : Writer
struct formatter : writer
{
	template<class T>
	void format_key(std::enable_if_t< std::is_integral_v<T>, T> a) {
		char buff[32];
		size_t l;
		if constexpr(std::is_signed_v<T>)
			l = snprintf(buff, sizeof buff, "%ill", a);
		else
			l = snprintf(buff, sizeof buff, "%ull", a);

		key(std::string_view(buff, l));
	}

	void format_key(std::string const& a) { key(a); }
	void format_key(std::string_view a) { key(a); }
	template<size_t N> void format_key(char const (&a)[N]) { key(std::string_view(a, N)); }

	template<class T>
	void format_value(std::enable_if_t< std::is_floating_point_v<T>, T> a) {
		char buff[32];
		size_t l = snprintf(buff, sizeof buff, "%f", a);
		value(std::string_view(buff, l));
	}
	template<class T>
	void format_value(std::enable_if_t< std::is_integral_v<T>, T> a) {
		char buff[32];
		size_t l;
		if constexpr(std::is_signed_v<T>)
			l = snprintf(buff, sizeof buff, "%ill", a);
		else
			l = snprintf(buff, sizeof buff, "%ull", a);

		value(std::string_view(buff, l));
	}
	template<class T>
	void encode_value(char const* data, size_t size, size_t wrap = 0) {
		std::string buff;
		buff.resize(util::base64_encode_length(size, util::base64_options::pad));
		util::base64_encode(data, size, buff.data(), util::base64_options::pad);

		tag("binary");
		value(buff, writer::block_fold|writer::wrap(wrap));
	}
};

} // yaml

} // pulmotor

#endif // PULMOTOR_YAML_EMITTER_HPP_
