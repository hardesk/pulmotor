#ifndef PULMOTOR_SERIALIZE_HPP_
#define PULMOTOR_SERIALIZE_HPP_

#include "archive.hpp"

namespace pulmotor {

template<class Arch, class T>
inline void serialize_data(Arch& ar, T const& obj);

struct access
{
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

        template<class T> struct has_load_construct<T,          std::void_t<decltype( load_construct(std::declval<A&>(), std::declval<T*&>(), 0U) )> >  : std::true_type {};
        template<class T> struct has_load_construct_mem<T,      std::void_t<decltype( T::load_construct(std::declval<A&>(), std::declval<T*&>(), 0U) )> >   : std::true_type {};

        template<class T> struct has_save_construct<T,          std::void_t<decltype( save_construct(std::declval<A&>(), std::declval<T&>(), 0U) )> >   : std::true_type {};
        template<class T> struct has_save_construct_mem<T,      std::void_t<decltype( std::declval<T>().save_construct(std::declval<A&>(), 0U) )> >   : std::true_type {};

        template<class T> struct count_serialize : std::integral_constant<unsigned,
			has_serialize<T>::value +
			has_serialize_version<T>::value +
			has_serialize_mem<T>::value +
			has_serialize_mem_version<T>::value> {};

        template<class T> struct count_load_construct : std::integral_constant<unsigned,
			has_load_construct<T>::value +
			has_load_construct_mem<T>::value> {};

        template<class T> struct count_save_construct : std::integral_constant<unsigned,
			has_save_construct<T>::value +
			has_save_construct_mem<T>::value> {};
    };

	template<class T>
	struct call
	{
		template<class A>
		static void do_serialize(A& ar, T& o, unsigned version) {
			using bare = std::decay_t<T>;
			PULMOTOR_SERIALIZE_ASSERT(detect<A>::template count_serialize<bare>::value > 0, "no suitable serialize method for type T was detected");
			PULMOTOR_SERIALIZE_ASSERT(detect<A>::template count_serialize<bare>::value == 1, "multiple serialize defined for T");
			if constexpr(detect<A>::template has_serialize_mem_version<bare>::value)
				o.serialize(ar, version);
			else if constexpr(detect<A>::template has_serialize_mem<bare>::value)
				o.serialize(ar);
			else if constexpr(detect<A>::template has_serialize_version<bare>::value)
				serialize(ar, o, version);
			else if constexpr(detect<A>::template has_serialize<bare>::value)
				serialize(ar, o);
		}

		template<class A>
		static void do_load_construct(A& ar, T* obj, unsigned version) {
			using bare = std::decay_t<T>;
			//PULMOTOR_SERIALIZE_ASSERT(detect<A>::template count_load_construct<bare>::value > 0, "no suitable load_construct method for type T was detected");
			//PULMOTOR_SERIALIZE_ASSERT(detect<A>::template count_load_construct<bare>::value == 1, "multiple load_construct defined for T");
			if constexpr(detect<A>::template has_load_construct_mem<bare>::value)
				T::load_construct(ar, obj, version);
			else if constexpr(detect<A>::template has_load_construct<bare>::value)
				load_construct<T>(ar, obj, version);
		}

		template<class A>
		static void do_save_construct(A& ar, T* o, unsigned version) {
			using bare = std::decay_t<T>;
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

/*template<class T>
inline constexpr unsigned get_version() {
	using bare = std::decay_t<T>;
	return version<bare>::value;
}

template<class T>
struct traits
{
	char const* compatible[];
	static constexpr unsigned version;
	static constexpr char const* name;
};

struct header
{
	u64 meta_offset, meta_size;
	u64 data_offset, data_size;
};

struct meta
{
	struct type
	{
		std::string name;
		std::string remap;
	};

	std::unordered_map<std::string, type> m_info;
};*/

// POINTER
template<class Tb> // Tb& obj ->> typedef To* Tb;
struct serialize_pointer
{
	using To = std::decay_t<std::remove_pointer_t<Tb>>;

	template<class A>
	static To* serialize_area(A& ar, void* area)
	{
		To* ptr = (To*)area;
		if constexpr(access::detect<A>::template count_load_construct<To>::value != 0) {
			unsigned arch_version = ar.process_prefix();
			access::call<To>::do_load_construct(ar, ptr, arch_version);
		} else {
			ptr = new (area) To();
			ar | *ptr;
		}
		return ptr;
	}

	template<class A, class F>
	static inline void load_withalloc (A& ar, To** pp, F const& f)
	{
		u32 id = 0;
		ar.read_basic(id);
		To* ptr = static_cast<To*>(ar.lookup_ptr(id)); 
		if (ptr == nullptr) {
			void* a = f();
			ptr = serialize_area(ar, a);
			ar.restore_ptr(id, ptr);
		}

		*pp = ptr;
	}

	template<class A>
	static inline void load_withplacement (A& ar, Tb* p)
	{
		u32 id = 0;
		ar.read_basic(id);
		To* ptr = (To*)ar.lookup_ptr(id); 
		if (ptr == nullptr) {
			ptr = serialize_area(ar, p);
			ar.restore_ptr(id, ptr);
		}
		else
		{
			// fatal error, placement-serialize has to serialize something
			assert(0);
		}
	}

	template<class A>
	static inline void load_ptr (A& ar, To** obj)
	{
#if 1
		load_withalloc(ar, obj, []() {
			To* ptr = nullptr;
			if constexpr(access::detect<A>::template count_load_construct<To>::value != 0)
				ptr = (To*)new std::aligned_storage<sizeof(To)>();
			else
				ptr = new To;
			return ptr;
		});
#else
		unsigned code_version = 0;//get_version<bare>();
		unsigned arch_version = ar.process_prefix();

		u32 id = 0;
		ar.read_basic(id);

		To* ptr = nullptr;
		if constexpr(access::detect<A>::template count_load_construct<To>::value != 0)
			ptr = (To*)new std::aligned_storage<sizeof(To)>();
		else
			ptr = new To;

		ptr = serialize_area(ar, ptr);
		ar.restore_ptr(id, obj);
#endif
	}

	// write object
	template<class A>
	static inline void save (A& ar, To const* obj)
	{
		To* mo = const_cast<To*>(obj);

		unsigned code_version = get_version<Tb>::value;
		u32 id = ar.lookup_id(mo);

		// write object if it has not been written out yet
		if (id == 0) {
			id = ar.reg_ptr(mo);
			ar.write_basic(id);
			if constexpr(access::detect<A>::template count_save_construct<To>::value != 0) {
				if (code_version != no_version)
					ar.template write_object_prefix<To>(*mo, code_version);
				access::call<To>::do_save_construct(ar, mo, code_version);
			} else
				//access::call<To>::do_serialize(ar, *const_cast<Tb&>(obj), code_version);
				ar | *mo;
		}
		else
		{
			ar.write_basic(id);
		}
	}
};

// ARRAY
template<class Tb>
struct serialize_array;

template<class Tb, size_t N>
struct serialize_array<Tb[N]>
{
	template<class A>
	static inline void doit (A& ar, Tb const (&arr)[N])
	{
		unsigned code_version = 0;//get_version<bare>();
		for (size_t i=0; i<N; ++i)
			ar | arr[i];
	}
};

// STRUCT
template<class Tb>
struct serialize_struct
{
	template<class A>
	static inline void doit (A& ar, Tb const& obj, true_t)
	{
		unsigned code_version = 0;//get_version<bare>();
		unsigned arch_version = ar.process_prefix();
		access::call<Tb>::do_serialize(ar, const_cast<Tb&>(obj), arch_version);
	}

	template<class A>
	static inline void doit (A& ar, Tb const& obj, false_t)
	{
		unsigned code_version = get_version<Tb>::value;
		if (code_version != no_version)
			ar.template write_object_prefix<Tb>(obj, code_version);
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
		if constexpr(!A::is_stream_aligned)
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

template<class T>
struct placement_t
{
	using type = typename std::remove_cv<T>::type;
	T* p;
};
template<class T = void>	struct is_placement : std::false_type {};
template<class T>			struct is_placement<placement_t<T>> : std::true_type {};
template<class T> inline placement_t<T> place(void* p) { return placement_t<typename std::remove_cv<T>::type>{(T*)p}; }

template<class T, class F>
struct alloc_t
{
	using type = typename std::remove_cv<T>::type;
	T** pp; // hold ptr-to-ptr (and void ref) to be confident that assignments are correct
	F f;
};
template<class T = void>	struct is_alloc : std::false_type {};
template<class T, class F>	struct is_alloc<alloc_t<T, F>> : std::true_type {};
template<class T, class F> inline alloc_t<T, F> alloc(T*& p, F const& f) { return alloc_t<typename std::remove_cv<T>::type, F>{&p, f}; }

template<class Arch, class T>
inline void serialize_data(Arch& ar, T const& obj)
{
	using bare = std::remove_cv_t<T>;
	//static_assert( !std::is_array<bare>::value, "is array"); 
	using reading = std::bool_constant<Arch::is_reading>;

	if constexpr(std::is_arithmetic<bare>::value)
		serialize_arithmetic<bare>::doit(ar, obj, reading());
	/*else if constexpr(is_nvp<bare>::value)
		serialize_nvp(ar, obj, code_version);
	*/else if constexpr(std::is_array<bare>::value)
		serialize_array<bare>::doit(ar, obj);
	else if constexpr(std::is_pointer<bare>::value)
		if constexpr(reading::value)
			serialize_pointer<bare>::load_ptr(ar, &const_cast<T&>(obj)); // To*& obj
		else
			serialize_pointer<bare>::save(ar, obj); // To*& obj
	else if constexpr(is_alloc<T>::value) {
		using To = typename T::type;
		if constexpr(reading::value)
			serialize_pointer<To>::load_withalloc(ar, obj.pp, obj.f);
		else
			serialize_pointer<To>::save(ar, *obj.pp); 
	}
	else if constexpr(is_placement<T>::value) {
		using To = typename T::type;
		if constexpr(reading::value)
			serialize_pointer<To>::load_withplacement(ar, obj.p);
		else
			serialize_pointer<To>::save(ar, obj.p); 
	}
	/*else if constexpr(std::is_enum<bare>::value)
		serialize_enum(ar, obj, code_version);
	else if constexpr(is_memblock<bare>::value)
		serialize_memblock(ar, obj, code_version);*/
	else if constexpr(std::is_class<bare>::value || std::is_union<bare>::value)
		serialize_struct<bare>::doit(ar, obj, reading());
	else
		static_assert(!std::is_same<T, T>::value, "attempting to serialize an unsupported type T");
}

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
	serialize_data(ar, obj);
	return ar;
}

}

#endif // PULMOTOR_SERIALIZE_HPP_
