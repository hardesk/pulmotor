#ifndef PULMOTOR_FWD_HPP_
#define PULMOTOR_FWD_HPP_

#include "pulmotor_config.hpp"
#include "pulmotor_types.hpp"
#include <tr1/type_traits>

//namespace std {
//	template<class T> class allocator;
//	template<class T, class Allocator> class vector;
//}

namespace pulmotor
{

template<class T> struct version;
template<class T> struct track_version;

struct pulmotor_archive;
//template<class T> struct ref_wrapper;
//template<class T>
//ref_wrapper<T> ref (T* t);

template<class T> struct ptr_address;
template<class T> inline ptr_address<typename std::tr1::remove_cv<T>::type> ptr (T*& p, size_t cnt = 1);
//template<class T> inline ptr_address<T> ptr (T const* const& p, size_t cnt);

class access;
struct blit_section;

template<class ArchiveT, class ObjectT> inline ArchiveT& blit (ArchiveT& ar, ObjectT& obj);

template<class ObjectT>
inline blit_section& operator | (blit_section& ar, ObjectT const& obj);
template<class ObjectT>
inline blit_section& operator & (blit_section& ar, ObjectT const& obj);

template<class ArchiveT, class T>
	inline typename pulmotor::enable_if<std::tr1::is_base_of<pulmotor_archive, ArchiveT>::value, ArchiveT>::type&
	operator& (ArchiveT& ar, T const& obj);

template<class ArchiveT, class T>
	inline typename pulmotor::enable_if<std::tr1::is_base_of<pulmotor_archive, ArchiveT>::value, ArchiveT>::type&
	operator| (ArchiveT& ar, T const& obj);

namespace util
{
	void* fixup_pointers_impl (pulmotor::blit_section_info* bsi);
	
	template<class T>
	inline T* fixup_pointers (pulmotor::blit_section_info* bsi);
	
	template<class T>
	inline T* fixup_pointers (void* dataWithHeader);	
	
	inline void fixup (pulmotor::blit_section_info* bsi);
	
	inline size_t write_file (pp_char const* name, u8 const* ptr, size_t size);
	
	template<class T>
	size_t write_file (pp_char const* name, T& root, target_traits const& tt, size_t sectionalign = 16);	
}

}

#endif // PULMOTOR_FWD_HPP_
