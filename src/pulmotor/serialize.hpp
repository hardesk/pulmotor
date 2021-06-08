#ifndef PULMOTOR_SERIALIZE_HPP_
#define PULMOTOR_SERIALIZE_HPP_

#include "archive.hpp"

namespace pulmotor {

template<class T, class D>	struct delete_holder		 { void de(T* p) const { d(p); } D const& d; };
template<class T> 			struct delete_holder<T,void> { void de(T*) const { } };

template<class T, class C, class D = delete_holder<T, void> >
struct ctor : D
{
	static_assert(std::is_same<T, typename std::remove_cv<T>::type>::value);

	using type = T;

	C const& cf;

	T** ptr;

	ctor(T** p, C const& c) : ptr(p), cf(c) {}
	ctor(T** p, C const& c, D const& d) : D(d), ptr(p), cf(c) {}

	template<class... Args> auto operator()(Args&&... args) const { return cf(args...); }
};

template<class T, class C, class D = delete_holder<T, void> >
struct ctor_pure : D
{
	static_assert(std::is_same<T, typename std::remove_cv<T>::type>::value);

	using type = T;
	C const& cf;

	ctor_pure(C const& c) : cf(c) {}
	ctor_pure(C const& c, D const& d) : D(d), cf(c) {}

	template<class... Args> auto operator()(Args&&... args) const { return cf(args...); }
};

template<class T>
struct fun_traits;

template<size_t I, class T, class... Args> struct arg_ii { using type = typename arg_ii<I-1, Args...>::type; };
template<class T, class... Args> struct arg_ii<0U, T, Args...> { using type = T; };
template<size_t I, class... Args> struct arg_i { using type = typename arg_ii<I, Args...>::type; };
template<size_t I> struct arg_i<I> { using type = void; };

template<class R, class T, class... Args>
struct fun_traits<R (T::*)(Args...)>
{
    static constexpr unsigned arity = sizeof...(Args);
    using return_type = R;
    using class_type = T;
    template<size_t I> using arg = typename arg_i<I, Args...>::type;
};

template<class R, class... Args>
struct fun_traits<R (*)(Args...)>
{
    static constexpr unsigned arity = sizeof...(Args);
    using return_type = R;
    template<size_t I> using arg = typename arg_i<I, Args...>::type;
};

struct access
{
	struct dummy_F { template<class... Args> auto operator()(Args&&... args); };

    template<class Ar>
    struct detect {

        template<class T, class = void> struct has_serialize					: std::false_type {};
        template<class T, class = void> struct has_serialize_mem				: std::false_type {};

        template<class T, class = void> struct has_serialize_version			: std::false_type {};
        template<class T, class = void> struct has_serialize_mem_version		: std::false_type {};

        template<class T, class = void> struct has_serialize_load				: std::false_type {};
        template<class T, class = void> struct has_serialize_load_mem			: std::false_type {};
        template<class T, class = void> struct has_serialize_load_version		: std::false_type {};
        template<class T, class = void> struct has_serialize_load_mem_version	: std::false_type {};

        template<class T, class = void> struct has_serialize_save				: std::false_type {};
        template<class T, class = void> struct has_serialize_save_mem			: std::false_type {};
        template<class T, class = void> struct has_serialize_save_version		: std::false_type {};
        template<class T, class = void> struct has_serialize_save_mem_version	: std::false_type {};

        template<class T, class = void> struct has_load_construct				: std::false_type {};
        template<class T, class = void> struct has_load_construct_mem			: std::false_type {};

        template<class T, class = void> struct has_save_construct				: std::false_type {};
        template<class T, class = void> struct has_save_construct_mem			: std::false_type {};

        template<class T> struct has_serialize<T,					std::void_t< decltype( serialize(std::declval<Ar&>(), std::declval<T&>()) )> > : std::true_type {};
        template<class T> struct has_serialize_mem<T,				std::void_t< decltype( std::declval<T&>().T::serialize(std::declval<Ar&>()) )> > : std::true_type {};

        template<class T> struct has_serialize_version<T,			std::void_t< decltype( serialize(std::declval<Ar&>(), std::declval<T&>(), 0U) )> > : std::true_type {};
        template<class T> struct has_serialize_mem_version<T,		std::void_t< decltype( std::declval<T&>().T::serialize(std::declval<Ar&>(), 0U) )> > : std::true_type {};

        template<class T> struct has_serialize_load<T,              std::void_t< decltype( serialize_load(std::declval<Ar&>(), std::declval<T&>()) )> > : std::true_type {};
        template<class T> struct has_serialize_load_mem<T,          std::void_t< decltype( std::declval<T&>().T::serialize_load(std::declval<Ar&>()) )> > : std::true_type {};
        template<class T> struct has_serialize_load_version<T,      std::void_t< decltype( serialize_load(std::declval<Ar&>(), std::declval<T&>(), 0U) )> > : std::true_type {};
        template<class T> struct has_serialize_load_mem_version<T,  std::void_t< decltype( std::declval<T&>().T::serialize_load(std::declval<Ar&>(), 0U) )> > : std::true_type {};

        template<class T> struct has_serialize_save<T,              std::void_t< decltype( serialize_save(std::declval<Ar&>(), std::declval<T&>()) )> > : std::true_type {};
        template<class T> struct has_serialize_save_mem<T,          std::void_t< decltype( std::declval<T&>().T::serialize_save(std::declval<Ar&>()) )> > : std::true_type {};
        template<class T> struct has_serialize_save_version<T,      std::void_t< decltype( serialize_save(std::declval<Ar&>(), std::declval<T&>(), 0U) )> > : std::true_type {};
        template<class T> struct has_serialize_save_mem_version<T,  std::void_t< decltype( std::declval<T&>().T::serialize_save(std::declval<Ar&>(), 0U) )> > : std::true_type {};

        template<class T> struct has_load_construct<T,				std::void_t< decltype( load_construct(std::declval<Ar&>(), std::declval<ctor<T,dummy_F>&>(), 0U) )> > : std::true_type {};
        template<class T> struct has_load_construct_mem<T,			std::void_t< decltype( T::load_construct(std::declval<Ar&>(), std::declval<ctor<T,dummy_F>&>(), 0U) )> > : std::true_type {};

        template<class T> struct has_save_construct<T,				std::void_t< decltype( save_construct(std::declval<Ar&>(), std::declval<T&>(), 0U) )> > : std::true_type {};
        template<class T> struct has_save_construct_mem<T,			std::void_t< decltype( std::declval<T>().T::save_construct(std::declval<Ar&>(), 0U) )> > : std::true_type {};

        template<class T> struct count_load_construct : std::integral_constant<unsigned,
			has_load_construct<T>::value +
			has_load_construct_mem<T>::value
		> {};

        template<class T> struct count_save_construct : std::integral_constant<unsigned,
			has_save_construct<T>::value +
			has_save_construct_mem<T>::value> {};
    };


	template<class Ar, class Tb>
	struct wants_construct : std::integral_constant<bool,
		(detect<Ar>::template count_load_construct<Tb>::value > 0)
	>
	{
		static_assert(std::is_base_of<archive, Ar>::value == true, "Ar must be an archive type");
		static_assert(std::is_same<Tb, std::remove_cv_t<Tb>>::value);
	};

	template<class T>
	struct call
	{
		template<class Ar>
		static void do_serialize(Ar& ar, T& o, unsigned version) {
			using bare = std::remove_cv_t<T>;
		   	if constexpr(detect<Ar>::template has_serialize_mem_version<bare>::value)
				o.serialize(ar, version);
			else if constexpr(detect<Ar>::template has_serialize_mem<bare>::value)
				o.serialize(ar);
			else if constexpr(detect<Ar>::template has_serialize_version<bare>::value)
				serialize(ar, o, version);
			else if constexpr(detect<Ar>::template has_serialize<bare>::value)
				serialize(ar, o);
			else if constexpr(Ar::is_reading) {
				if constexpr(detect<Ar>::template has_serialize_load_mem<bare>::value)
					o.serialize_load(ar);
				else if constexpr(detect<Ar>::template has_serialize_load_mem_version<bare>::value)
					o.serialize_load(ar, version);
				else if constexpr(detect<Ar>::template has_serialize_load<bare>::value)
					serialize_load(ar, o);
				else if constexpr(detect<Ar>::template has_serialize_load_version<bare>::value)
					serialize_load(ar, o, version);
			} else if constexpr(Ar::is_writing) {
				if constexpr(detect<Ar>::template has_serialize_save_mem<bare>::value)
					o.serialize_save(ar);
				else if constexpr(detect<Ar>::template has_serialize_save_mem_version<bare>::value)
					o.serialize_save(ar, version);
				else if constexpr(detect<Ar>::template has_serialize_save<bare>::value)
					serialize_save(ar, o);
				else if constexpr(detect<Ar>::template has_serialize_save_version<bare>::value)
					serialize_save(ar, o, version);
			}
		}

		template<class Ar, class F, class D>
		static void do_load_construct(Ar& ar, ctor<T, F, D> const& c, unsigned version) {
			using bare = typename ctor<T, F>::type;//std::remove_cv_t<T>;
			if constexpr(detect<Ar>::template has_load_construct_mem<bare>::value)
				T::load_construct(ar, c, version);
			else if constexpr(detect<Ar>::template has_load_construct<bare>::value)
				load_construct(ar, c, version);
			/*else if constexpr(detect<Ar>::template count_serialize<bare>::value > 0) // fallback to using serialize
			{
				bare* p = c();
				ar | *p;
			}*/ else
				static_assert(!std::is_same<T, T>::value, "attempting to call load_construct but no suitable was declared, and no serialize fallback detected");
		}

		template<class Ar>
		static void do_save_construct(Ar& ar, T* o, unsigned version) {
			using bare = std::remove_cv_t<T>;
			PULMOTOR_SERIALIZE_ASSERT(detect<Ar>::template count_save_construct<bare>::value > 0, "no suitable save_construct method for type T was detected");
			PULMOTOR_SERIALIZE_ASSERT(detect<Ar>::template count_save_construct<bare>::value == 1, "multiple save_construct defined for T");
			if constexpr(detect<Ar>::template has_save_construct_mem<bare>::value)
				o->save_construct(ar, version);
			/*else if constexpr(detect<Ar>::template has_save_construct<bare>::value)
				save_construct<T>(ar, o, version);*/
		}
	};
};

template<class Ar, class Tb>
struct wants_construct : access::wants_construct<Ar, Tb> {};

template<class T>
struct version { static constexpr unsigned value = 0; };

template<class T>
struct placement_t
{
	using type = typename std::remove_cv<T>::type;
	T* p;
};
template<class T = void>	struct is_placement : std::false_type {};
template<class T>			struct is_placement<placement_t<T>> : std::true_type {};

template<class T>
inline placement_t<T>
place(void* p)
{ return placement_t<typename std::remove_cv<T>::type>{(T*)p}; }

template<class T, class F>	struct deallocate_holder		 { void de(T* p) const { d(p); } F const& d; };
template<class T> 			struct deallocate_holder<T,void> { void de(T*) const {} };

template<class T, class AF, class DH = deallocate_holder<T,void> >
struct alloc_t : DH
{
	alloc_t(T** p, AF const& af) : pp(p), f(af) {}
	alloc_t(T** p, AF const& af, DH const& dh) : DH(dh), pp(p), f(af) {}

	static_assert(std::is_same<T, typename std::remove_cv<T>::type>::value);
	using type = T;
	T** pp;
	AF f;
};

template<class T = void>			  struct is_alloc : std::false_type {};
template<class T, class AF, class DF> struct is_alloc<alloc_t<T, AF, DF>> : std::true_type {};

template<class T, class AF>
inline alloc_t<T, AF>
alloc(T*& p, AF const& af)
{
	using bare = typename std::remove_cv<T>::type;
	return alloc_t<bare, AF>{&p, af};
}

template<class T, class AF, class DF>
inline alloc_t<T, AF, deallocate_holder<T, DF>>
alloc(T*& p, AF const& af, DF const& df)
{
	using bare = typename std::remove_cv<T>::type;
	return alloc_t<bare, AF, deallocate_holder<bare, DF>>{&p, af, deallocate_holder<bare, DF>{df}};
}

template<class T>			struct base_t { using type = T; T* p; };
template<class T = void>	struct is_base : std::false_type {};
template<class T>			struct is_base<base_t<T>> : std::true_type {};
template<class B, class T>
inline base_t<B> base(T* p) {
	static_assert(!std::is_same<B, T>::value, "infinite recursion. argument to pulmotor::base should be a base class type");
	static_assert(std::is_base_of<B, T>::value, "'base' argument is not a base class");
	return base_t<B>{p};
}


template<class T, class C>
inline ctor_pure<T, C>
construct(C const& f)
{ return ctor_pure<std::remove_cv_t<T>, C>(f); }

template<class T, class C>
inline ctor<std::remove_cv_t<T>, C>
construct(T*& p, C const& f)
{ return ctor<std::remove_cv_t<T>, C>(&p, f); }

template<class T, class C, class D>
inline ctor<T, C, delete_holder<std::remove_cv_t<T>, D>>
construct(T*& p, C const& f, D const& d)
{ return ctor<std::remove_cv_t<T>, C, delete_holder<T, D>>(&p, f, delete_holder<T, D>{d}); }

template<class T = void>			struct is_construct					: std::false_type {};
template<class T, class C, class D>	struct is_construct<ctor<T, C, D>>	: std::true_type {};

template<class T = void>			struct is_construct_pure						: std::false_type {};
template<class T, class C, class D>	struct is_construct_pure<ctor_pure<T, C, D>>	: std::true_type {};

template<class T>
struct array_ref
{
	T* data;
	size_t size;
};

template<class T = void>	struct is_array_ref : std::false_type {};
template<class T>			struct is_array_ref<array_ref<T>> : std::true_type {};

template<class T>
inline array_ref<T>
array(T const* p, size_t s)
{ return array_ref<T>(const_cast<T*>(p), s); }

template<class Tb>
struct logic
{
	template<class Ar>
	static void s_pointer(Ar& ar, Tb* p, object_version version, std::true_type) // wants-load-save = true
	{
		if constexpr(Ar::is_reading) {
			if (!version.is_nullptr()) {
				auto f = [p](auto&&... args) { new (p) Tb(std::forward<decltype(args)>(args)...); };
				access::call<Tb>::do_load_construct(ar, ctor<Tb, decltype(f)>(&p, f), version);
			}
		} else if constexpr( Ar::is_writing ) {
			if (p)
				access::call<Tb>::do_save_construct(ar, p, version);
		}
	}

	template<class Ar>
	static void s_pointer(Ar& ar, Tb* p, object_version version, std::false_type) // wants-load-save = false
	{
		if (p)
			logic<Tb>::s_serializable(ar, *p);
	}

	template<class F, class DF>
	static void destruct_or_alloc(Tb*& o, bool doalloc, F const& alloc_fun, DF const& dealloc_fun )
	{
		if (o) {
			o->~Tb();
			if (!doalloc) {
				dealloc_fun(o);
				o = nullptr;
			}
		} else if (doalloc)
			o = (Tb*)alloc_fun();
	}

	template<class F, class DF>
	static void alloc_construct_if_null(Tb*& o, bool doalloc, F const& alloc_fun, DF const& dealloc_fun)
	{
		if (!o && doalloc) {
			void* p = alloc_fun();
			o = new (p) Tb();
		} else if (o && !doalloc) {
			dealloc_fun(o);
			o = nullptr;
		}
	}

	template<bool WantConstruct, class AF, class DF>
	static void prepare_area(Tb** pp, bool doalloc, AF af, DF df)
	{
		if constexpr(WantConstruct)
			logic<Tb>::destruct_or_alloc(*pp, doalloc, af, df);
		else
			logic<Tb>::alloc_construct_if_null(*pp, doalloc, af, df);
	}

	template<class Ar>
	static void s_serializable(Ar& ar, Tb& o)
	{
		if constexpr(std::is_pointer_v<Tb>) {
			using Tp = typename std::remove_pointer<Tb>::type;
			using wc = typename access::wants_construct<Ar, Tp>::type;

			auto new_aligned = []() { return new typename std::aligned_storage<sizeof(Tp)>::type; };
			auto delete_obj = [](Tp* p) { delete p; };

			object_version v = logic<Tp>::s_version(ar, o);
			if constexpr(Ar::is_reading)
				logic<Tp>::template prepare_area<wc::value>(&o, !v.is_nullptr(), new_aligned, delete_obj);
			logic<Tp>::s_pointer(ar, o, v, wc());
		} else if constexpr(is_alloc<Tb>::value) {
			using Tp = typename Tb::type;
			using wc = typename access::wants_construct<Ar, Tp>::type;

			auto alloc_f = [&]() { return o.f(); };
			auto dealloc_f = [&](Tp* p) { o.de(p); };

			object_version v = logic<Tp>::s_version(ar, *o.pp);
			if constexpr(Ar::is_reading)
				logic<Tp>::template prepare_area<wc::value>(o.pp, !v.is_nullptr(), alloc_f, dealloc_f);
			logic<Tp>::s_pointer(ar, *o.pp, v, wc());
		} else if constexpr(is_placement<Tb>::value) {
			using Tp = typename Tb::type;
			using wc = typename access::wants_construct<Ar, Tp>::type;

			object_version v = logic<Tp>::s_version(ar, o.p);
			// TODO: pass a bool with "placement_t" that specifies if the are holds an object
			// if constexpr(Ar::is_reading)
			// logic<Tp>::template prepare_area<wc::value>(o.p, Ar::is_reading && !v.is_nullptr(, [o.p]() { return p; }, [](auto){} );

			logic<Tp>::s_pointer(ar, o.p, v, wc());
		} else if constexpr(is_construct<Tb>::value || is_construct_pure<Tb>::value) {
			using To = typename Tb::type;

			object_version version;

			if constexpr(Ar::is_reading) {
				version = ar.process_prefix();
				if constexpr(!is_construct_pure<Tb>::value) {
					assert(o.ptr);
					if(*o.ptr) {
						o.de(*o.ptr);
						*o.ptr = nullptr;
					}
					if (!version.is_nullptr()) {
						auto assign_ptr = [&o](auto&&... args) {
							*o.ptr = reinterpret_cast<To*>(o.cf(std::forward<decltype(args)>(args)...));
						};
						access::call<To>::do_load_construct(ar, ctor<To, decltype(assign_ptr)>(o.ptr, assign_ptr), version);
					}
				} else {
					auto cc = [&o](auto&&... args) { o.cf(std::forward<decltype(args)>(args)...); };
					access::call<To>::do_load_construct(ar, ctor<To, decltype(cc)>(nullptr, cc), version);
				}
			} else if constexpr(Ar::is_writing) {
				version = object_version { get_version<Tb>::value };
				if (!o.ptr)
					throw std::invalid_argument("bad construct when writing stream");
				ar.template write_object_prefix<To>(*o.ptr, version);
				if (*o.ptr)
					access::call<To>::do_save_construct(ar, *o.ptr, version);
			}
		} else if constexpr(std::is_array_v<Tb>) {
			using Ta = typename std::remove_extent<Tb>::type;

			if constexpr(std::rank<Ta>::value == 0) {
				if constexpr(std::is_arithmetic<Ta>::value || std::is_enum<Ta>::value)
					logic<Tb>::s_primitive_array(ar, o);
				else if constexpr(std::is_pointer<Ta>::value) {
					static_assert(!std::is_same<Ta, Ta>::value, "array of pointers is not supported yet");
				} else {
					object_version v = logic<Tb>::s_version(ar, &o); // <<------- what to pass here?
					logic<Tb>::s_struct_array(ar, o, v);
				}
			} else {
				using Ta = typename std::remove_extent<Tb>::type;
				for (size_t i=0; i<std::extent<Tb>::value; ++i)
					logic<Ta>::s_serializable( ar, o[i] );
			}
		} else if constexpr(is_base<Tb>::value) {
			using Tp = typename Tb::type;
			object_version v = logic<Tp>::s_version(ar, o.p);
			logic<Tp>::s_struct(ar, *o.p, v);
		} else if constexpr(std::is_arithmetic<Tb>::value || std::is_enum<Tb>::value) {
			s_primitive(ar, o);
		} else if constexpr(std::is_class<Tb>::value || std::is_union<Tb>::value) {
			object_version v = s_version(ar, &o);
			s_struct(ar, o, v);
		}
	}

	template<class Ar>
	static object_version s_version(Ar& ar, Tb* o)
	{
		object_version v;
		if constexpr(Ar::is_reading)
			v = ar.process_prefix();
		else {
			v = object_version { get_version<Tb>::value };
			if (v != no_version)
				ar.template write_object_prefix<Tb>(o, v);
		}
		return v;
	}

	template<class Ar>
	static void s_struct(Ar& ar, Tb& o, object_version v) {
		if constexpr(Ar::is_reading) {
			static_assert(access::wants_construct<Ar, Tb>::value == false, "a value that wants construct should not be serializing here");
			access::call<Tb>::do_serialize(ar, const_cast<Tb&>(o), v);
		} else if constexpr(Ar::is_writing) {
			if constexpr(access::wants_construct<Ar, Tb>::value)
				access::call<Tb>::do_save_construct(ar, &const_cast<Tb&>(o), v);
			else
				access::call<Tb>::do_serialize(ar, const_cast<Tb&>(o), v);
		}
	}

	template<class Ar>
	static void s_struct_array(Ar& ar, Tb& o, object_version v) {
		constexpr size_t N = std::extent<Tb>::value;
		using Ta = typename std::remove_all_extents<Tb>::type;
		for (size_t i=0; i<N; ++i) {
			logic<Ta>::s_struct(ar, o[i], v);
		}
	}

	template<class Ar>
	static void s_primitive_array(Ar& ar, Tb& o) {
		constexpr size_t N = std::extent<Tb>::value;
		using To = typename std::remove_all_extents<Tb>::type;
		ar.align_stream(sizeof(Tb));
		for (size_t i=0; i<N; ++i) {
			if constexpr(Ar::is_reading) {
				ar.read_data(o, N * sizeof(To));
			} else {
				ar.write_data(o, N * sizeof(To));
			}
		}
	}

	//using Tu = std::underlying_type_t<Tb>;
	//logic<Tu>::s_primitive(ar, (Tu&)o);
	template<class Ar>
	static void s_primitive(Ar& ar, Tb& o) {
		ar.align_stream(sizeof(Tb));
		if constexpr(Ar::is_reading) {
			ar.read_basic(const_cast<Tb&> (o));
		} else if constexpr(Ar::is_writing) {
			ar.write_basic(o);
		}
	}
};

template<class ArchiveT, class T>
inline typename is_archive_if<ArchiveT>::type&
operator| (ArchiveT& ar, T const& obj)
{
	using Tb = std::remove_cv_t<T>;
	logic<Tb>::s_serializable(ar, const_cast<T&>(obj));
	return ar;
}

} // pulmotor

#endif // PULMOTOR_SERIALIZE_HPP_
