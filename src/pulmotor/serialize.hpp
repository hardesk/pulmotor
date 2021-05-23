#ifndef PULMOTOR_SERIALIZE_HPP_
#define PULMOTOR_SERIALIZE_HPP_

#include "archive.hpp"

namespace pulmotor {

template<class Arch, class T>
inline void serialize_data(Arch& ar, T const& obj);

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
struct ctor_emplace : D
{
	static_assert(std::is_same<T, typename std::remove_cv<T>::type>::value);

	using type = T;
	C const& cf;

	ctor_emplace(C const& c) : cf(c) {}
	ctor_emplace(C const& c, D const& d) : D(d), cf(c) {}

	template<class... Args> auto operator()(Args&&... args) const { return cf(args...); }
};

struct access
{
	struct dummy_F { template<class... Args> auto operator()(Args&&... args); };

    template<class A>
    struct detect {

        template<class T, class = void> struct has_serialize : std::false_type {};
        template<class T, class = void> struct has_serialize_mem : std::false_type {};

        template<class T, class = void> struct has_serialize_version : std::false_type {};
        template<class T, class = void> struct has_serialize_mem_version : std::false_type {};

        template<class T, class = void> struct has_load_construct : std::false_type {};
        template<class T, class = void> struct has_load_construct_mem : std::false_type {};

        template<class T, class = void> struct has_save_construct : std::false_type {};
        template<class T, class = void> struct has_save_construct_mem : std::false_type {};

        template<class T> struct has_serialize<T,               std::void_t<decltype( serialize(std::declval<A&>(), std::declval<T&>()) )> >          : std::true_type {};
        template<class T> struct has_serialize_mem<T,           std::void_t<decltype( std::declval<T&>().serialize(std::declval<A&>()) )> >           : std::true_type {};

        template<class T> struct has_serialize_version<T,       std::void_t<decltype( serialize(std::declval<A&>(), std::declval<T&>(), 0U) )> >      : std::true_type {};
        template<class T> struct has_serialize_mem_version<T,   std::void_t<decltype( std::declval<T&>().serialize(std::declval<A&>(), 0U) )> >       : std::true_type {};

        template<class T> struct has_load_construct<T, std::void_t<decltype( load_construct(std::declval<A&>(), std::declval<ctor<T,dummy_F>&>(), 0U) )> >  : std::true_type {};
        template<class T> struct has_load_construct_mem<T, std::void_t<decltype( T::load_construct(std::declval<A&>(), std::declval<ctor<T,dummy_F>&>(), 0U) )> >   : std::true_type {};

        template<class T> struct has_save_construct<T,          std::void_t<decltype( save_construct(std::declval<A&>(), std::declval<T&>(), 0U) )> >   : std::true_type {};
        template<class T> struct has_save_construct_mem<T,      std::void_t<decltype( std::declval<T>().save_construct(std::declval<A&>(), 0U) )> >   : std::true_type {};

        template<class T> struct count_serialize : std::integral_constant<unsigned,
			has_serialize<T>::value +
			has_serialize_version<T>::value +
			has_serialize_mem<T>::value +
			has_serialize_mem_version<T>::value> {};

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
		static_assert(std::is_base_of<pulmotor_archive, Ar>::value == true, "Ar must be archive type");
		static_assert(std::is_same<Tb, std::remove_cv_t<Tb>>::value);
	};

	template<class T>
	struct call
	{
		template<class A>
		static void do_serialize(A& ar, T& o, unsigned version) {
			using bare = std::remove_cv_t<T>;
			PULMOTOR_SERIALIZE_ASSERT(detect<A>::template count_serialize<bare>::value > 0, "no suitable serialize method for type T was detected");
			PULMOTOR_SERIALIZE_ASSERT(detect<A>::template count_serialize<bare>::value <= 1, "multiple serialize defined for T");
			if constexpr(detect<A>::template has_serialize_mem_version<bare>::value)
				o.serialize(ar, version);
			else if constexpr(detect<A>::template has_serialize_mem<bare>::value)
				o.serialize(ar);
			else if constexpr(detect<A>::template has_serialize_version<bare>::value)
				serialize(ar, o, version);
			else if constexpr(detect<A>::template has_serialize<bare>::value)
				serialize(ar, o);
		}

		template<class A, class F, class D>
		static void do_load_construct(A& ar, ctor<T, F, D> const& c, unsigned version) {
			using bare = typename ctor<T, F>::type;//std::remove_cv_t<T>;
			//PULMOTOR_SERIALIZE_ASSERT(detect<A>::template count_load_construct<bare, F>::value > 0, "no suitable load_construct method for type T was detected");
			//PULMOTOR_SERIALIZE_ASSERT(detect<A>::template count_load_construct<bare, F>::value == 1, "multiple load_construct defined for T");
			if constexpr(detect<A>::template has_load_construct_mem<bare>::value)
				T::load_construct(ar, c, version);
			else if constexpr(detect<A>::template has_load_construct<bare>::value)
				load_construct(ar, c, version);
			/*else if constexpr(detect<A>::template count_serialize<bare>::value > 0) // fallback to using serialize
			{
				bare* p = c();
				ar | *p;
			}*/ else
				
				static_assert(!std::is_same<T, T>::value, "attempting to call load_construct but no suitable is declared, and not serialize fallback detected");
		}

		template<class A>
		static void do_save_construct(A& ar, T* o, unsigned version) {
			using bare = std::remove_cv_t<T>;
			PULMOTOR_SERIALIZE_ASSERT(detect<A>::template count_save_construct<bare>::value > 0, "no suitable save_construct method for type T was detected");
			PULMOTOR_SERIALIZE_ASSERT(detect<A>::template count_save_construct<bare>::value == 1, "multiple save_construct defined for T");
			if constexpr(detect<A>::template has_save_construct_mem<bare>::value)
				o->save_construct(ar, version);
			/*else if constexpr(detect<A>::template has_save_construct<bare>::value)
				save_construct<T>(ar, o, version);*/
		}
	};
};

#if 0
// Forward to in-class function if a global one is not available
struct CLASS_DOES_NOT_HAVE_AN_ARCHIVE_FUNCTION_OR_MEMBER {};

template<class ArchiveT, class T>
inline void has_archive_check_impl (T& obj, true_t)
{}

template<class ArchiveT, class T>
inline void has_archive_check_impl (T& obj, false_t)
{
	typedef std::integral_constant<bool, has_archive_fun<ArchiveT, T>::value> has_archive_t;
	typename boost::mpl::if_<has_archive_t, char, std::pair<T, CLASS_DOES_NOT_HAVE_AN_ARCHIVE_FUNCTION_OR_MEMBER****************> >::type* test = "";
	(void)test;
}

template<class ArchiveT, class T>
inline void call_archive_member_impl (ArchiveT& ar, T& obj, unsigned long version, std::integral_constant<int, 0>)
{
	typedef std::integral_constant<bool, has_archive_fun<ArchiveT, T>::value> has_archive_t;
	typename boost::mpl::if_<has_archive_t, char, std::pair<T, CLASS_DOES_NOT_HAVE_AN_ARCHIVE_FUNCTION_OR_MEMBER****************> >::type* test = "";
	(void)test;
}

template<class ArchiveT, class T>
inline void call_archive_member_impl (ArchiveT& ar, T const& obj, unsigned long version, std::integral_constant<int, 1>)
{
	const_cast<T&>(obj).archive (ar, (unsigned)version);
	//	access::call_archive (ar, obj, (unsigned)version);
}

template<class ArchiveT, class T>
inline void call_archive_member_impl (ArchiveT& ar, T const& obj, unsigned long version, std::integral_constant<int, 2>)
{
	const_cast<T&>(obj).serialize (ar, (unsigned)version);
	//	access::call_serialize(ar, obj, (unsigned)version);
}

template<class ArchiveT, class T>
inline void archive (ArchiveT& ar, T& obj, unsigned long version)
{
	typedef std::integral_constant<bool, has_archive_fun<ArchiveT, T>::value> has_archive_t;
	typedef std::integral_constant<bool, has_serialize_fun<ArchiveT, T>::value> has_serialize_t;
	
	typedef std::integral_constant<int, has_archive_t::value ? 1 : has_serialize_t::value ? 2 : 0> has_arch_or_ser_t;
	//typedef std::integral_constant<int, 1> has_arch_or_ser_t;
	
	//has_archive_check_impl<ArchiveT> (obj, has_archive_t());
	//access::call_archive (ar, obj, version);
	//	call_archive_member_impl (ar, obj, version, has_arch_or_ser_t());
	call_archive_member_impl (ar, obj, version, has_arch_or_ser_t());
}
	
#define REDIRECT_ARCHIVE(ar, obj, version) \
	archive (ar, *const_cast<typename std::remove_cv<T>::type*>(&obj), (unsigned)version)
	
//template<class ArchiveT, class T>
//inline void redirect_archive (ArchiveT& ar, T const& obj, unsigned version)
//{
//	typedef typename std::remove_cv<T>::type clean_t;
//	archive (ar, *const_cast<clean_t*>(&obj), version);
//}

// T is a class (READING)
template<class ArchiveT, class T>
inline void dispatch_archive_impl (ArchiveT& ar, T const& obj, unsigned version, true_t, std::integral_constant<int, 1>)
{
	version_t file_version = version;
	//	if (get_version<T>::value != pulmotor::version_dont_track)
	ar.template do_version<T> (file_version);
	ar.begin_object();
	REDIRECT_ARCHIVE (ar, obj, file_version);
	ar.end_object();
}

// T is a class (WRITING)
template<class ArchiveT, class T>
inline void dispatch_archive_impl (ArchiveT& ar, T const& obj, unsigned version, false_t, std::integral_constant<int, 1>)
{
	version_t file_version = version;
	//	if (get_version<T>::value != pulmotor::version_dont_track)
	ar.template do_version<T> (file_version);
	ar.begin_object();
	REDIRECT_ARCHIVE (ar, obj, version);
	ar.end_object();
}

// T is a primitive
template<class ArchiveT, class T>
inline void dispatch_archive_impl (ArchiveT& ar, T const& obj, unsigned version, true_t, std::integral_constant<int, 0>)
{
	const int Align = std::is_floating_point<T>::value ? sizeof(T) : 1;
	ar.template read_basic<Align> (const_cast<T&> (obj));
}

template<class ArchiveT, class T>
inline void dispatch_archive_impl (ArchiveT& ar, T const& obj, unsigned version, false_t, std::integral_constant<int, 0>)
{
	const int Align = std::is_floating_point<T>::value ? sizeof(T) : 1;
	ar.template write_basic<Align> (obj);
}

// T is a wrapped pointer
	template<class ArchiveT, class T, bool AnyContruct>
	inline void memblock_archive_impl (ArchiveT& ar, memblock_t<AnyContruct, T> const& mb, unsigned version, true_t, true_t /*is_fundamental*/) {
		ar.read_data ((void*)mb.addr, mb.count * sizeof(T));
	}
	
	template<class ArchiveT, class T, bool AnyContruct>
	inline void memblock_archive_impl (ArchiveT& ar, memblock_t<AnyContruct, T> const& mb, unsigned version, false_t, true_t /*is_fundamental*/) {
		ar.write_data ((void*)mb.addr, mb.count * sizeof(T));
	}
	
	template<class ArchiveT, class T>
	inline void memblock_archive_impl (ArchiveT& ar, memblock_t<true, T> const& mb, unsigned version, true_t /*is_reading*/, false_t) {
		version_t file_version = version;
//		if (get_version<T>::value != version_dont_track)
		ar.template do_version<memblock_t<true, T>> (file_version);
		
		ar.begin_array();
		for (size_t i=0; i<mb.count; ++i)
			archive<1>(ar, *mb.ptr_at(i));
		ar.end_array();
	}

	
	template<class ArchiveT, class T>
	inline void memblock_archive_impl (ArchiveT& ar, memblock_t<false, T> const& mb, unsigned version, true_t /*is_reading*/, false_t) {
		version_t file_version = version;
		//		if (get_version<T>::value != version_dont_track)
		ar.template do_version<memblock_t<false, T>> (file_version);
		
		ar.begin_array();
		for (size_t i=0; i<mb.count; ++i)
			select_archive_construct(ar, mb.ptr_at(i), version);
		ar.end_array();
	}
	
	template<class ArchiveT, class T, bool Constructed>
	inline void memblock_archive_impl (ArchiveT& ar, memblock_t<Constructed, T> const& mb, unsigned version, false_t /*is_reading*/, false_t /*is_fundamental*/) {
		version_t file_version = version;
//		if (get_version<T>::value != version_dont_track)
		ar.template do_version<memblock_t<Constructed, T>> (file_version);
		
		ar.begin_array();
		for (size_t i=0; i<mb.count; ++i)
			archive<1>(ar, *mb.ptr_at(i));
		ar.end_array();
	}
	
	
template<class ArchiveT, bool Constructed, class T, class AnyT>
inline void dispatch_archive_impl (ArchiveT& ar, memblock_t<Constructed, T> const& w, unsigned version, AnyT, std::integral_constant<int, 2>)
{
	if (w.addr != 0 && w.count != 0)
	{
		typedef std::integral_constant<bool, ArchiveT::is_reading> is_reading_t;
		typedef std::integral_constant<bool, std::is_fundamental<T>::value> is_fund_t;
		
		memblock_archive_impl (ar, w, version, is_reading_t(), is_fund_t());
	}
}

/*template<class ArchiveT, class T>
inline void dispatch_archive_impl (ArchiveT& ar, memblock_t<T> const& w, unsigned version, false_t, std::integral_constant<int, 2>)
{
	if (w.addr != 0 && w.count != 0)
		ar.write_data ((void*)w.addr, w.count * sizeof(T));
}*/
	
// T is an array
template<class ArchiveT, class T, int N, class AnyT>
inline void dispatch_archive_impl (ArchiveT& ar, T const (&a)[N], unsigned version, AnyT, std::integral_constant<int, 3>)
{
	ar.begin_array();
	for (size_t i=0; i<N; ++i)
		archive (ar, a[i]);
	//		operator| (ar, a[i]);
	ar.end_array();
}

// Default version for constructing an object
template<class ArchiveT, class T>
inline void archive_construct (ArchiveT& ar, T* p, unsigned long version)
{
	// by default we create an object and serialize its members
	new (p)T();
	archive (ar, *p, version);
}

template<class ArchiveT, class T>
inline void invoke_archive_construct(ArchiveT& ar, T* o, unsigned version, std::true_type /* has member construct*/)
{
	T::archive_construct (ar, o, version);
}

#if 1
	template<class ArchiveT, class T>
inline void invoke_archive_construct(ArchiveT& ar, T* o, unsigned version, std::false_type /* non member construct*/)
{
	archive_construct (ar, o, version);
}
#endif
	
template<class ArchiveT, class T>
inline void select_archive_construct(ArchiveT& ar, T* o, unsigned version)
{
	// here we call one of invoke_archive_construct versions. In case where member function is not available
	// the call ends up either in default archive_construct, or in user-defined archive_construct according
	// to 'version' type matching
	typedef std::integral_constant<bool, has_static_archive_construct<ArchiveT, T>::value> has_memfun_t;
	
	//	char CLASS_DOEST_HAVE_SAVE_CONTRUCT_FUNCTION[has_memfun_t::value ? 1 : -1];
	
	invoke_archive_construct (ar, o, version, has_memfun_t());
}
	

//template<class ArchiveT, class T>
//inline void dispatch_archive_impl (ArchiveT& ar, T const* p, unsigned version, true_t, std::integral_constant<int, 4>)
//{
//	select_archive_construct (ar, p, version);
//}

// Default version for saving `construct' parameters
template<class ArchiveT, class T>
inline void archive_save_construct (ArchiveT& ar, T const* p, unsigned long version)
{
	// by default we call into straight serialization
	archive (ar, *p, version);
}

template<class ArchiveT, class T>
inline void invoke_archive_save_construct(ArchiveT& ar, T* o, unsigned version, std::true_type)
{
	o->archive_save_construct (ar, version);
}

#if 1
	template<class ArchiveT, class T>
inline void invoke_archive_save_construct(ArchiveT& ar, T* o, unsigned version, std::false_type /* non member construct*/)
{
	archive_save_construct (ar, o, version);
}
#endif

	
template<class ArchiveT, class T>
inline void select_archive_save_construct(ArchiveT& ar, T* o, unsigned version)
{
	// save_contruct is selected same as select_archive_construct
	typedef std::integral_constant<bool, has_archive_save_construct<ArchiveT, T>::value> has_memfun_t;
	
	//	typedef T CLASS_DOEST_HAVE_SAVE_CONTRUCT_FUNCTION[has_memfun_t::value ? 1 : -1];
	
	invoke_archive_save_construct (ar, o, version, has_memfun_t());
}


	// Writing an object trough pointer
template<class ArchiveT, class T>
inline void dispatch_archive_impl (ArchiveT& ar, T const* p, unsigned version, false_t, std::integral_constant<int, 4>)
{
	select_archive_save_construct(ar, p, version);
}
	
	
// Reading an object into a pointer (need to create object first)
template<class ArchiveT, class T>
inline void dispatch_archive_impl (ArchiveT& ar, T* const& p, unsigned version, true_t, std::integral_constant<int, 4>)
{
	struct alignas(T) object_holder { char data_[sizeof(T)]; };
	object_holder* ps = new object_holder;
//	printf("allocated %u at %p\n", (int)sizeof(object_holder), ps);
	select_archive_construct(ar, (T*)ps, version);
	const_cast<T*&> (p) = (T*)ps;
}


// T is enum
//template<class ArchiveT, class T>
//inline void dispatch_archive_impl (ArchiveT& ar, T const& v, unsigned version, AnyT, std::integral_constant<int, 5>)
//{
//	if (w.addr != 0 && w.count != 0)
//		ar.read_data ((void*)w.addr, w.count * sizeof(T));
//}
	
template<class ArchiveT, class T>
inline void dispatch_archive_impl (ArchiveT& ar, nv_impl<T> const& p, unsigned version, true_t /*is_reading*/, std::integral_constant<int, 5>)
{
	// TODO: lookup by name?
//	printf("de-serializing %s %s\n", p.name, typeid(p.obj).name());
	ar.object_name (p.name);
	//	operator| (ar, p.obj);
	archive (ar, p.obj);
	ar.object_name (NULL);
}
	
template<class ArchiveT, class T>
inline void dispatch_archive_impl (ArchiveT& ar, nv_impl<T> const& p, unsigned version, false_t /*is_reading*/, std::integral_constant<int, 5>)
{
	// if archive supports name-value-pair natively, pass on, otherwise...
	// if we want to save name, do it as separate fields
	// printf("serializing %s %s\n", p.name, typeid(p.obj).name());
	ar.object_name (p.name);
	//	operator| (ar, p.obj);
	archive (ar, p.obj);
	ar.object_name (NULL);
}
	
template<class ArchiveT, bool IsContextCarrier>
struct reduce_archive_helper { typedef ArchiveT type; };

template<class ArchiveT>
struct reduce_archive_helper<ArchiveT, true> { typedef typename ArchiveT::base_t type; };

template<class ArchiveT>
struct reduce_archive
{
	typedef typename reduce_archive_helper<ArchiveT, is_context_carrier<ArchiveT>::value>::type type;
};
#endif 

template<class T>
struct version { static constexpr unsigned value = 0; };

// ARRAY (NATIVE)
template<class Tb>
struct serialize_native_array;

template<class Tb, size_t N>
struct serialize_native_array<Tb[N]>
{
	template<class A>
	static inline void doit (A& ar, Tb const (&arr)[N])
	{
		for (size_t i=0; i<N; ++i)
			ar | arr[i];
	}
};

// ARRAY
template<class Tb>
struct serialize_array
{
	template<class A>
	static inline void doit (A& ar, Tb* p, size_t size)
	{
		// todo: optimize for basic types
		for (size_t i=0; i<size; ++i)
			ar | p[i];
	}
};

// STRUCT
template<class Tb>
struct serialize_struct
{
	template<class A>
	static inline void doit (A& ar, Tb const& obj, true_t)
	{
		object_version arch_version = ar.process_prefix();
		static_assert(access::wants_construct<A, Tb>::value == false, "a value that wants construct should not be serializing here");
		access::call<Tb>::do_serialize(ar, const_cast<Tb&>(obj), arch_version);
	}

	template<class A>
	static inline void doit (A& ar, Tb const& obj, false_t)
	{
		unsigned code_version = get_version<Tb>::value;
		if (code_version != no_version)
			ar.template write_object_prefix<Tb>(&obj, code_version);
		if constexpr(access::wants_construct<A, Tb>::value)
			access::call<Tb>::do_save_construct(ar, &const_cast<Tb&>(obj), code_version);
		else
			access::call<Tb>::do_serialize(ar, const_cast<Tb&>(obj), code_version);
	}
};

// ARITHMETIC
template<class Tb>
struct serialize_arithmetic
{
	template<class A>
	static void doit (A& ar, Tb const& obj, true_t)
	{
		//if constexpr(!A::is_stream_aligned)
		ar.align_stream(sizeof(Tb));
		ar.read_basic(const_cast<Tb&> (obj));
	}

	template<class ArchiveT>
	static void doit (ArchiveT& ar, Tb obj, false_t)
	{
		ar.align_stream(sizeof(Tb));
		ar.write_basic(obj);
	}
};

// ENUM
template<class Tb>
struct serialize_enum
{
	using Tu = std::underlying_type_t<Tb>;

	template<class A>
	static void doit (A& ar, Tb const& obj, true_t)
	{
		ar.align_stream(sizeof(Tu));
		ar.read_basic((Tu&)obj);
	}

	template<class ArchiveT>
	static void doit (ArchiveT& ar, Tb obj, false_t)
	{
		ar.align_stream(sizeof(Tb));
		ar.write_basic((Tu&)obj);
	}
};

template<class T>
struct placement_t
{
	using type = typename std::remove_cv<T>::type;
	T* p;
};
template<class T = void>	struct is_placement : std::false_type {};
template<class T>			struct is_placement<placement_t<T>> : std::true_type {};
template<class T> inline placement_t<T> place(void* p) { return placement_t<typename std::remove_cv<T>::type>{(T*)p}; }

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

template<class T, class AF> 		  inline alloc_t<T, AF>     alloc(T*& p, AF const& af)
{ using bare = typename std::remove_cv<T>::type; return alloc_t<bare, AF>{&p, af}; }

template<class T, class AF, class DF> inline alloc_t<T, AF, deallocate_holder<T, DF>> alloc(T*& p, AF const& af, DF const& df)
{ using bare = typename std::remove_cv<T>::type; return alloc_t<bare, AF, deallocate_holder<bare, DF>>{&p, af, deallocate_holder<bare, DF>{df}}; }



template<class T, class C>
inline ctor_emplace<T, C> construct(C const& f)
{ return ctor_emplace<std::remove_cv_t<T>, C>(f); }

template<class T, class C>
inline ctor<std::remove_cv_t<T>, C> construct(T*& p, C const& f)
{ return ctor<std::remove_cv_t<T>, C>(&p, f); }

template<class T, class C, class D>
inline ctor<T, C, delete_holder<std::remove_cv_t<T>, D>> construct(T*& p, C const& f, D const& d)
{ return ctor<std::remove_cv_t<T>, C, delete_holder<T, D>>(&p, f, delete_holder<T, D>{d}); }

template<class T = void>			struct is_construct : std::false_type {};
template<class T, class C, class D>	struct is_construct<ctor<T, C, D>> : std::true_type {};

template<class T = void>			struct is_construct_emplace : std::false_type {};
template<class T, class C, class D>	struct is_construct_emplace<ctor_emplace<T, C, D>> : std::true_type {};

template<class T>
struct array_ref
{
	T* data;
	size_t size;
};

template<class T = void>	struct is_array_ref : std::false_type {};
template<class T>			struct is_array_ref<array_ref<T>> : std::true_type {};
template<class T> inline array_ref<T> array(T const* p, size_t s) { return array_ref<T>(const_cast<T*>(p), s); }

template<class T>
struct is_data : std::bool_constant<
	std::is_arithmetic<T>::value ||
	std::is_array<T>::value ||
	std::is_enum<T>::value ||
	std::is_class<T>::value || std::is_union<T>::value>
{};


template<class Tb>
struct logic
{
// <serializable>	:= ATTACH_CONTEXT ? ( <data> | <pointer> | <special> )
// <data>			:= arithmetic# [serialize] | <native-array> | enum# [serialize] | <struct> 
// <native-array>	:= ( <data> | <pointer> ) +
// <struct>		:= --> call custom function // serializable //
// <special>		:= PLACE(area) <data> | CREATE(ptr|nullptr) <struct> (1) | ARRAY <data>
// <pointer>		:= PTR ([alloc-memory] [ctor] <=data> | ALLOC [ctor] <=data> | CREATE [save-load-=data] )

	template<class Ar>
	static void pointer(Ar& ar, Tb* p, object_version version, std::true_type) // wants-load-save = true
	{
		if constexpr(Ar::is_reading) {
			if (!version.is_nullptr()) {
				auto f = [p](auto&&... args) { new (p) Tb(std::forward<decltype(args)>(args)...); };
				access::call<Tb>::do_load_construct(ar, ctor<Tb, decltype(f)>(&p, f), version);
			}
		} else if constexpr( Ar::is_writing ) {
			ar.template write_object_prefix<Tb>(p, version);
			if (p)
				access::call<Tb>::do_save_construct(ar, p, version);
		}
	}

	template<class Ar>
	static void pointer(Ar& ar, Tb* p, object_version version, std::false_type) // wants-load-save = false
	{
		if (p)
			logic<Tb>::serializable(ar, *p);
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
			o = new (o) Tb();
		} else if (o && !doalloc) {
			dealloc_fun(o);
			o = nullptr;
		}
	}

	template<class Ar>
	static void serializable(Ar& ar, Tb& o) 
	{
		if constexpr(std::is_pointer_v<Tb>) {

			using Tp = typename std::remove_pointer<Tb>::type;
			using wc = typename access::wants_construct<Ar, Tp>::type;
			static_assert(wc::value == true);

			auto new_aligned = []() { return new typename std::aligned_storage<sizeof(Tp)>::type; };
			auto delete_obj = [](Tp* p) { delete p; };

			object_version version;

			if constexpr(Ar::is_reading) {
				version = ar.process_prefix();
				if constexpr(wc::value)
					logic<Tp>::destruct_or_alloc(o, !version.is_nullptr(), new_aligned, delete_obj);
				else
					logic<Tp>::alloc_construct_if_null(o, !version.is_nullptr(), new_aligned, delete_obj);
			} else
				version = object_version { get_version<Tb>::value };
			logic<Tp>::pointer(ar, o, version, wc());
		} else if constexpr(is_alloc<Tb>::value) {
			using Tp = typename Tb::type;

			auto alloc_f = [&]() { return o.f(); };
			auto dealloc_f = [&](Tp* p) { o.de(p); };

			object_version version;

			using wc = typename access::wants_construct<Ar, Tp>::type;
			if constexpr(Ar::is_reading) {
				version = ar.process_prefix();
				if constexpr(wc::value)
					logic<Tp>::destruct_or_alloc( *o.pp, !version.is_nullptr(), alloc_f, dealloc_f );
				else
					logic<Tp>::alloc_construct_if_null(o, !version.is_nullptr(), alloc_f, dealloc_f);
			} else
				version = object_version { get_version<Tb>::value };
			logic<Tp>::pointer(ar, *o.pp, version, wc());
		} else if constexpr(is_placement<Tb>::value) {
			using Tp = typename Tb::type;
			using wc = typename access::wants_construct<Ar, Tp>::type;

			object_version version;
			if constexpr(Ar::is_reading)
				version = ar.process_prefix();
			else
				version = object_version { get_version<Tb>::value };

			logic<Tp>::pointer(ar, o.p, version, wc());
		} else if constexpr(is_construct<Tb>::value || is_construct_emplace<Tb>::value) {
			using To = typename Tb::type;

			object_version version;

			if constexpr(Ar::is_reading) {
				version = ar.process_prefix();
				if constexpr(!is_construct_emplace<Tb>::value) {
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
		}
		else if constexpr(is_data<Tb>::value)
			data(ar, o);
	}

//	template<class Ar>
//	static void array_helper(Ar& ar, Tb* o, size_t size)
//	{
//		for(size_t i=0; i<size; ++i)
//			logic<Tb>::serializable( ar, o[i]);
//	}

	template<class Ar>
	static void data(Ar& ar, Tb& o)
	{
		using reading = std::bool_constant<Ar::is_reading>;
		if constexpr(std::is_arithmetic_v<Tb>)
			serialize_arithmetic<Tb>::doit(ar, o, reading());
		else if constexpr(std::is_array_v<Tb>) {
			using Ta = typename std::remove_extent<Tb>::type;
			// logic<Ta>::array_helper(ar, o, std::extent<Tb>::value);
			// TODO: optimize when Tb is trivial
			for (size_t i=0; i<std::extent<Tb>::value; ++i)
				logic<Ta>::serializable( ar, o[i] );
		} else if constexpr(std::is_enum_v<Tb>)
			serialize_enum<Tb>::doit(ar, o, reading());
		else if constexpr(std::is_class<Tb>::value || std::is_union<Tb>::value)
			serialize_struct<Tb>::doit(ar, o, reading());
		//else
		//	static_assert(!std::is_same<Tb, Tb>::value, "attempting to serialize an unsupported type T");
	}

};

/*
template<class A, class T, class... Args>
inline void archive (ArchiveT& ar, object_context<T, Args...> const& ctx)
{
	typedef object_context<T, Args...> ctx_t;
	typedef typename reduce_archive<ArchiveT>::type base_t;
	
	archive_with_context<base_t, typename ctx_t::data_t> arch_ctx (ar, ctx.m_data);
	archive (arch_ctx, ctx.m_object);
}
	
template<bool IsReading> struct impl_archive_as {
	template<class ArchiveT, class AsT, class ActualT>
	static void arch (ArchiveT& ar, as_holder<AsT, ActualT> const& ash) {
		AsT asObj;
		archive (ar, asObj);
		ash.m_actual = static_cast<ActualT>(asObj);
	}
};
template<> struct impl_archive_as<false> {
	template<class ArchiveT, class AsT, class ActualT>
	static void arch (ArchiveT& ar, as_holder<AsT, ActualT> const& ash) {
		AsT asObj = static_cast<AsT> (ash.m_actual);
		archive (ar, asObj);
	}
};

template<class ArchiveT, class AsT, class ActualT>
inline void archive (ArchiveT& ar, as_holder<AsT, ActualT> const& ash)
{
	impl_archive_as<ArchiveT::is_reading>::arch (ar, ash);
}

template<class ArchiveT, class T>
inline typename pulmotor::enable_if<std::is_base_of<pulmotor_archive, ArchiveT>::value, ArchiveT>::type&
operator& (ArchiveT& ar, T const& obj)
{
	arch (ar, obj);
	return ar;
}
*/

template<class ArchiveT, class T>
inline typename pulmotor::enable_if<std::is_base_of<pulmotor_archive, ArchiveT>::value, ArchiveT>::type&
operator| (ArchiveT& ar, T const& obj)
{
	using Tb = std::remove_cv_t<T>;
	logic<Tb>::serializable(ar, const_cast<T&>(obj));
	return ar;
}

}

#endif // PULMOTOR_SERIALIZE_HPP_
