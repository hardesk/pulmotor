#ifndef PULMOTOR_TRAITS_HPP_
#define PULMOTOR_TRAITS_HPP_

#include "pulmotor_types.hpp"
#include "access.hpp"

namespace pulmotor
{

struct binary_nameless_traits {

	template<class Ar, class T, class Context>
	static void do_struct(Ar& ar, T& o, object_meta version, Context& ctx) {
		access::call<T>::pick_serialize(ar, o, version);
	}

	template<class Ar, class T, class Context>
	static void do_struct(Ar& ar, nv_t<T>& o, object_meta version, Context& ctx) {
		serialize_with_context(ar, o.obj, ctx);
	}

	template<class Ar, class T, class Context>
	static void do_primitive(Ar& ar, T& o, Context& ctx) {
		if constexpr(Ar::is_reading) {
			ar.template read_basic_aligned<0>(const_cast<T&> (o));
		} else if constexpr(Ar::is_writing) {
			ar.template write_basic_aligned<0>(o);
		}
	}

	template<class Ar, class T, class Context>
	static void do_primitive_array(Ar& ar, T* o, size_t size, Context& ctx) {
		if constexpr(Ar::is_reading)
			ar.read_data(o, size * sizeof(T));
		else
			ar.write_data_aligned(o, size * sizeof(T), alignof (T));
	}

	template<class Ar, class T, class Context>
	static void do_struct_array(Ar& ar, T* o, size_t size, object_meta version, Context& ctx) {
		for (size_t i=0; i<size; ++i)
			access::call<T>::pick_serialize(ar, o[i], version);
//			serialize_with_context(ar, array(o, size), ctx);
	}

};

} // pulmotor



#include "yaml_emitter.hpp"

namespace pulmotor {

struct yaml_write_traits {
	// EXPECT C:
	// u8 yaml_flags; // block_literal|block_fold, eol_clip|eol_strip|eol_keep, single_quoted|double_quoted, flow
	// u8 base64:1;
	// u8 base64_wrap = 0;
	//
	template<class C>
	struct named {
		std::string_view name;
		C& ctx;
	};

	template<class Ar, class T, class Context>
	static void do_struct(Ar& ar, nv_t<T>& o, object_meta version, Context& ctx) {
		named<Context> n { std::string_view{ o.name, o.name_length }, ctx };
		serialize_with_context(ar, o.obj, n);
	}

	template<class Ar, class T, class Context>
	static void do_struct(Ar& ar, T& o, object_meta version, Context& ctx) {
		ar.begin_object(ctx);
		access::call<T>::pick_serialize(ar, o, version);
		ar.end_container();
	}

	template<class Ar, class T, class Context>
	static void do_struct(Ar& ar, T& o, object_meta version, named<Context>& n) {
		ar.begin_object(n.ctx);
		access::call<T>::pick_serialize(ar, o, version);
		ar.end_container();
	}

	template<class Ar, class T, class Context>
	static void do_primitive(Ar& ar, T& o, Context& ctx) {
		ar.write_basic(o, ctx);
	}

	template<class Ar, class T, class Context>
	static void do_primitive(Ar& ar, T& o, named<Context>& n) {
		ar.write_key(n.name);
		ar.write_basic(o, n.ctx);
	}

	template<class Ar, class T, class Context>
	static void do_struct_array(Ar& ar, T const* o, size_t size, Context& ctx) {
		ar.begin_array(ctx);
		for (size_t i=0; i<size; ++i)
			serialize_with_context(ar, array(o, size), ctx);
		ar.end_container();
	}

	template<class Ar, class T, class Context>
	static void do_struct_array(Ar& ar, T const* o, size_t size, named<Context>& n) {
		ar.write_key(n.name);
		do_struct_array(ar, o, size, n.ctx);
	}

	template<class Ar, class T, class Context>
	static void do_primitive_array(Ar& ar, T const* o, size_t size, Context& ctx) {
		if (ctx.base64)
			ar.encode_array(ctx, o, size);
		else {
			ar.begin_array(ctx);
			for (size_t i=0; i<size; ++i) {
				ar.write_basic(o[i]);
			}
			ar.end_container();
		}
	}

	template<class Ar, class T, class Context>
	static void do_primitive_array(Ar& ar, T const* o, size_t size, named<Context>& n) {
		ar.write_key(n.name);
		do_primitive_array(ar, o, size, n.ctx);
	}

// 	void encode_value(char const* data, size_t size, size_t wrap = 0) {
};

} // pulmotor

#endif // PULMOTOR_TRAITS_HPP_
