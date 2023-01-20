#ifndef PULMOTOR_SERIALIZE_HPP_
#define PULMOTOR_SERIALIZE_HPP_

#include "archive.hpp"
#include "access.hpp"

namespace pulmotor {

// template<class ArchiveT, class T, class C>
// inline ArchiveT&
// serialize_with_context (ArchiveT& ar, T const& obj, C& ctx);


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

template<class T>			struct base_t { using base_type = T; T* p; };
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
    using type = T;
    T* data;
    size_t size;
};

template<class T = void>	struct is_array_ref : std::false_type {};
template<class T>			struct is_array_ref<array_ref<T>> : std::true_type {};

template<class T>
inline array_ref<T>
array(T const* p, size_t s)
{ return array_ref<T>{(T*)p, s}; }

template<class S, class Q>
struct vu_t
{
    using quantity_type = Q;
    using serialize_type = S;
    Q* q;
};
template<class S, class Q>
vu_t<S, Q> vu(Q& q) { return vu_t<S, Q>{&q}; }

template<class T = void>	struct is_vu				: std::false_type {};
template<class S, class Q>	struct is_vu<vu_t<S, Q>>	: std::true_type {};

struct empty_context {};

template<class Tb>
struct logic
{
    template<class Ar>
    static void s_pointer(Ar& ar, Tb* p, prefix pre, std::true_type) // wants-load-save = true
    {
        if constexpr(Ar::is_reading) {
            if (!pre.is_nullptr()) {
                auto f = [p](auto&&... args) { new (p) Tb(std::forward<decltype(args)>(args)...); };
                access::call<Tb>::do_load_construct(ar, ctor<Tb, decltype(f)>(&p, f), pre.version);
            }
        } else if constexpr( Ar::is_writing ) {
            if (p)
                access::call<Tb>::do_save_construct(ar, p, pre.version);
        }
    }

    template<class Ar>
    static void s_pointer(Ar& ar, Tb* p, prefix pre, std::false_type) // wants-load-save = false
    {
        if (p)
        {
            // TODO: check if pointer is already serialized
            // write data only if unwritten, otherwise store a reference to the written object
            logic<Tb>::s_serializable(ar, *p);
        }
    }

    template<class F, class DF>
    static void destruct_or_alloc(Tb*& o, bool doalloc, F const& alloc_fun, DF const& dealloc_fun)
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
        if (false) {
        // } else if constexpr(std::is_pointer_v<Tb>) {
        //     using Tp = typename std::remove_const<typename std::remove_pointer<Tb>::type>::type;
        //     using wc = typename access::wants_construct<Ar, Tp>::type;

        //     struct alignas(Tp) Ta { u8 data[sizeof(Tp)]; };

        //     auto new_aligned = []() { return new Ta; };
        //     auto delete_obj = [](Tp* p) { delete p; };

        //     object_meta v = logic<Tp>::s_version(ar, const_cast<Tp*>(o), true);
        //     if constexpr(Ar::is_reading)
        //         logic<Tp>::template prepare_area<wc::value>(reinterpret_cast<Tp**>(&o), !v.is_nullptr(), new_aligned, delete_obj);
        //     logic<Tp>::s_pointer(ar, const_cast<Tp*>(o), v, wc());
        // } else if constexpr(is_alloc<Tb>::value) {
        //     using Tp = typename Tb::type;
        //     using wc = typename access::wants_construct<Ar, Tp>::type;

        //     auto alloc_f = [&]() { return o.f(); };
        //     auto dealloc_f = [&](Tp* p) { o.de(p); };

        //     object_meta v = logic<Tp>::s_version(ar, *o.pp, true);
        //     if constexpr(Ar::is_reading)
        //         logic<Tp>::template prepare_area<wc::value>(o.pp, !v.is_nullptr(), alloc_f, dealloc_f);
        //     logic<Tp>::s_pointer(ar, *o.pp, v, wc());
        // } else if constexpr(is_placement<Tb>::value) {
        //     using Tp = typename Tb::type;
        //     using wc = typename access::wants_construct<Ar, Tp>::type;

        //     object_meta v = logic<Tp>::s_version(ar, o.p, true);
        //     // TODO: expand "placement_t" with a bool that specifies whether the area holds an object (as it may
        //     // be initialized or uninitialized).
        //     // if constexpr(Ar::is_reading)
        //     // logic<Tp>::template prepare_area<wc::value>(o.p, Ar::is_reading && !v.is_nullptr(, [o.p]() { return p; }, [](auto){} );

        //     logic<Tp>::s_pointer(ar, o.p, v, wc());
        // } else if constexpr(is_construct<Tb>::value || is_construct_pure<Tb>::value) {
        //     using To = typename Tb::type;

        //     object_meta version;

        //     if constexpr(Ar::is_reading) {
        //         version = ar.process_prefix();
        //         if constexpr(!is_construct_pure<Tb>::value) {
        //             assert(o.ptr);
        //             if(*o.ptr) {
        //                 o.de(*o.ptr);
        //                 *o.ptr = nullptr;
        //             }
        //             if (!version.is_nullptr()) {
        //                 auto assign_ptr = [&o](auto&&... args) {
        //                     *o.ptr = reinterpret_cast<To*>(o.cf(std::forward<decltype(args)>(args)...));
        //                 };
        //                 access::call<To>::do_load_construct(ar, ctor<To, decltype(assign_ptr)>(o.ptr, assign_ptr), version);
        //             }
        //         } else {
        //             auto cc = [&o](auto&&... args) { o.cf(std::forward<decltype(args)>(args)...); };
        //             access::call<To>::do_load_construct(ar, ctor<To, decltype(cc)>(nullptr, cc), version);
        //         }
        //     } else if constexpr(Ar::is_writing) {
        //         version = object_meta { get_meta<Tb>::value };

        //         // we still must write flags (and thus version) even when 'no-version' is asked. in such case set it to 0.
        //         if (!version.include_version()) version.set_version(version_default);
        //         if (!o.ptr)
        //             throw_error(err_invalid_value,
        //                 ssprintf("bad construct (%s) when writing stream", typeid(o.ptr).name()).c_str(), __FILE__,
        //                 text_location{__LINE__, 0});

        //         ar.template write_object_prefix<To>(*o.ptr, version);
        //         if (*o.ptr)
        //             access::call<To>::do_save_construct(ar, *o.ptr, version);
        //     }
        } else if constexpr(std::is_array_v<Tb>) {
            using Ta = typename std::remove_extent<Tb>::type;

            if constexpr(std::rank<Ta>::value == 0) {
                constexpr size_t Na = std::extent<Tb>::value;
                using To = typename std::remove_all_extents<Tb>::type;
                if constexpr(std::is_arithmetic<Ta>::value || std::is_enum<Ta>::value) {
                    logic<To>::s_primitive_array(ar, o, Na);
                } else if constexpr(std::is_pointer<Ta>::value) {
                    static_assert(!std::is_same<Ta, Ta>::value, "array of pointers is not supported");
                } else {
                    // using Ta = typename std::remove_all_extents<Tb>::type;
                    prefix p = logic<To>::s_prefix(ar);
                    logic<To>::s_struct_array(ar, o, Na, p);
                }
            } else {
                using Ta = typename std::remove_extent<Tb>::type;
                // 	ar.begin_array();
                for (size_t i=0; i<std::extent<Tb>::value; ++i)
                    logic<Ta>::s_serializable(ar, o[i]);
                // 	ar.end_array();
            }
        // } else if constexpr(is_array_ref<Tb>::value) {
        //     // ar.begin_array ();
        //     using Ta = typename Tb::type;
        //     if constexpr(std::is_arithmetic<Ta>::value || std::is_enum<Ta>::value)
        //         logic<Ta>::s_primitive_array(ar, o.data, o.size);
        //     else if constexpr(std::is_pointer<Ta>::value) {
        //         static_assert(!std::is_same<Ta, Ta>::value, "array of pointers is not supported");
        //     } else {
        //         object_meta v = logic<Ta>::s_version(ar, o.data, false);
        //         logic<Ta>::s_struct_array(ar, o.data, o.size, v);
        //     }
        } else if constexpr(is_base<Tb>::value) {
            using Tbase = typename Tb::base_type;
            prefix p = logic<Tbase>::s_prefix(ar);
            logic<Tbase>::s_struct(ar, *o.p, p);
        } else if constexpr(is_vu<Tb>::value) {
            using Ts = typename Tb::serialize_type;
            logic<Ts>::s_vu(ar, *o.q);
        } else if constexpr(is_mod_list<Tb>::value) {
            using Tv = typename Tb::value_type;
            apply_mod_list(ar, o);
            logic<Tv>::s_serializable(ar, o.value);

            // ar | wrap( 64, base64( array(a, 100) ));
            // ar | array(a, 100) * base64<yaml_archive>() * wrap(64)
            // ar | array(a, 100) * yaml_base64() * wrap(64)
            // ar | array(a, 100) * yaml_base64() * wrap_sticky(64)

        } else if constexpr(std::is_arithmetic<Tb>::value || std::is_enum<Tb>::value) {
            s_primitive(ar, o);
        } else if constexpr(std::is_class<Tb>::value || std::is_union<Tb>::value) {
            constexpr prefix code = type_util<Tb>::extract_prefix();
            ar.begin_object();
            if constexpr (code.store()) {
                prefix_ext* ppx = nullptr;
#if PULMOTOR_DEBUG_SERIALIZE
                prefix_ext px = prefix_ext{ typeid(Tb).name() };
                ppx = &px;
#endif
                if constexpr (Ar::is_writing) {
                    ar.write_prefix(code, ppx);
                    s_struct(ar, o, code);
                } else if constexpr (Ar::is_reading) {
                    prefix p = ar.read_prefix(ppx);
                    s_struct(ar, o, p);
                }
            } else {
                s_struct(ar, o, code);
            }
            ar.end_object();
        }
    }

    template<class Ar, class Tq>
    static void s_vu(Ar& ar, Tq& q) {
        if constexpr(Ar::is_reading) {
            ar.template read_vu<Tb>(q);
        } else if constexpr(Ar::is_writing) {
            ar.template write_vu<Tb>(q);
        }
    }

    template<class Ar>
    static prefix s_prefix(Ar& ar) {
        if constexpr (Ar::is_reading) {
            return ar.read_prefix(nullptr);
        } else {
            prefix p = type_util<Tb>::extract_prefix();
            ar.write_prefix(p, nullptr);
            return p;
        }
    }

    template<class Ar>
    static void s_struct(Ar& ar, Tb& o, prefix px) {
        access::call<Tb>::pick_serialize(ar, o, px.version);
    }

    template<class Ar>
    static void s_struct_array(Ar& ar, Tb* o, size_t size, prefix px) {
        for (size_t i=0; i<size; ++i) {
            logic<Tb>::s_struct(ar, o[i], px);
        }
    }

    template<class Ar>
    static void s_primitive_array(Ar& ar, Tb* o, size_t size) {
        if constexpr(Ar::is_reading)
        	ar.template read_data(o, size);
        else
        	ar.template write_data(o, size);
    }

    template<class Ar>
    static void s_primitive(Ar& ar, Tb& o) {
        if constexpr(Ar::is_reading) {
        	ar.template read_single(o);
        } else if constexpr(Ar::is_writing) {
        	ar.template write_single(o);
        }
    }
};

template<class ArchiveT, class T>
inline typename is_archive_if<ArchiveT>::type&
operator| (ArchiveT& ar, T const& obj) {
    using Tb = std::remove_cv_t<T>;
    logic<Tb>::s_serializable(ar, const_cast<Tb&>(obj));
    return ar;
}

template<class ArchiveT, class T>
inline typename is_archive_if<ArchiveT>::type&
operator| (ArchiveT& ar, T& obj) {
    using Tb = std::remove_cv_t<T>;
    logic<Tb>::s_serializable(ar, obj);
    return ar;
}

template<class ArchiveT, class T, class D>
inline typename is_archive_if<ArchiveT>::type&
operator| (ArchiveT& ar, context<T, D> const& ctx) {
    using Tb = std::remove_cv_t<T>;
    logic<Tb>::s_serializable(ar, const_cast<Tb&>(ctx.obj), ctx.data);
    return ar;
}

// template<class ArchiveT, class T, class Context>
// inline ArchiveT&
// serialize_with_context (ArchiveT& ar, T const& obj, Context& ctx) {
//     using Tb = std::remove_cv_t<T>;
//     logic<Tb>::s_serializable(ar, const_cast<Tb&>(obj), ctx);
//     return ar;
// }

// template<class ArchiveT, class T>
// inline typename is_archive_if<ArchiveT>::type&
// operator& (ArchiveT& ar, T const& obj) {
// 	using Tb = std::remove_cv_t<T>;
// 	empty_context ctx {};
// 	logic<Tb>::s_serializable(ar, const_cast<T&>(obj), ctx);
// 	return ar;
// }

template<class Ar, class T>
inline void serialize(Ar& ar, nv_t<T>& nv)
{
    ar.write_name(nv.get_nameview());
    ar | nv.obj;
    // ar.set_name(std::string_view(nv.name, nv.name_length));
    // ar | nv.obj;
    // ar.set_name(std::string_view());
}


} // pulmotor

#endif // PULMOTOR_SERIALIZE_HPP_
