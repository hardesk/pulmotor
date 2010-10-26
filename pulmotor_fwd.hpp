#ifndef PULMOTOR_FWD_HPP_
#define PULMOTOR_FWD_HPP_

#include "pulmotor_config.hpp"
#include "pulmotor_types.hpp"

//namespace std {
//	template<class T> class allocator;
//	template<class T, class Allocator> class vector;
//}

namespace pulmotor
{

template<class T> struct version;
template<class T> struct track_version;
//template<class T> struct ref_wrapper;
//template<class T>
//ref_wrapper<T> ref (T* t);

template<class T> struct ptr_address;
template<class T> inline ptr_address<T> ptr (T*& p, size_t cnt);
template<class T> inline ptr_address<T> ptr (T const* const& p, size_t cnt);

class access;
struct blit_section;

template<class ArchiveT, class ObjectT> inline ArchiveT& blit (ArchiveT& ar, ObjectT& obj);
template<class ArchiveT, class ObjectT> inline ArchiveT& operator | (ArchiveT& ar, ObjectT const& obj);

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
