#ifndef PULMOTOR_ACCESS_HPP_
#define PULMOTOR_ACCESS_HPP_

#include <type_traits>
#include "pulmotor_types.hpp"

namespace pulmotor
{

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
			has_load_construct_mem<T>::value> {};

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
				else
					static_assert(!std::is_same<T, T>::value, "serialize_load (or serialize) function for T could not be found");
			} else if constexpr(Ar::is_writing) {
				if constexpr(detect<Ar>::template has_serialize_save_mem<bare>::value)
					o.serialize_save(ar);
				else if constexpr(detect<Ar>::template has_serialize_save_mem_version<bare>::value)
					o.serialize_save(ar, version);
				else if constexpr(detect<Ar>::template has_serialize_save<bare>::value)
					serialize_save(ar, o);
				else if constexpr(detect<Ar>::template has_serialize_save_version<bare>::value)
					serialize_save(ar, o, version);
				else
					static_assert(!std::is_same<T, T>::value, "serialize_save (or serialize) function for T could not be found");
			} else
				static_assert(!std::is_same<T, T>::value, "serialize function for T could not be found");
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

		template<class Ar>
		static void pick_serialize(Ar& ar, T& o, object_meta v) {
			if constexpr(Ar::is_reading) {
				static_assert(access::wants_construct<Ar, T>::value == false, "a value that wants construct should not be serializing here");
				access::call<T>::do_serialize(ar, const_cast<T&>(o), v);
			} else if constexpr(Ar::is_writing) {
				if constexpr(access::wants_construct<Ar, T>::value)
					access::call<T>::do_save_construct(ar, &const_cast<T&>(o), v);
				else
					access::call<T>::do_serialize(ar, const_cast<T&>(o), v);
			}
		}

	};
};

} // pulmotor

#endif //  PULMOTOR_ACCESS_HPP_
