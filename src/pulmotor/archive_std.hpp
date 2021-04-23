#ifndef PULMOTOR_ARCHIVE_STD_HPP_
#define PULMOTOR_ARCHIVE_STD_HPP_

#include "archive.hpp"

namespace pulmotor
{
	
template<class ArchiveT, class F, class S>
inline void archive (ArchiveT& ar, std::pair<F, S>& v, unsigned aver) {
	ar | v.first | v.second;
}
	

template<class ArchiveT, class T, class AllocatorT>
void archive (ArchiveT& ar, std::list<T, AllocatorT>& v, unsigned version)
{
	u32 sz = v.size ();
	ar | sz;

	if (ArchiveT::is_reading) {
		for (size_t i=0; i<sz; ++i)
		{
			T e;
			ar | e;
			v.emplace_back (e);
		}
	} else {
		for (typename std::list<T, AllocatorT>::iterator it = v.begin (), end = v.end (); it != end; ++it)
			ar | *it;
	}
}
	
template<bool TypeIsFundamental, bool IsReading>
struct impl_archive_stdvector {
	template<class ArchiveT, class T, class AllocatorT>
	static void arch (ArchiveT& ar, std::vector<T, AllocatorT>& v, size_t sz, unsigned version) {
		if (ArchiveT::is_reading)
			v.resize (sz);
		
		ar & pulmotor::memblock (&*v.begin(), sz);
	}
};

template<>
struct impl_archive_stdvector<false, true> {
	template<class ArchiveT, class T, class AllocatorT>
	static void arch (ArchiveT& ar, std::vector<T, AllocatorT>& v, size_t sz, unsigned version) {
		v.reserve (sz);
		for (size_t i=0; i<sz; ++i) {
			T t;
			ar | t;
			v.emplace_back(std::move(t));
		}
	}
};

template<>
struct impl_archive_stdvector<false, false> {
	template<class ArchiveT, class T, class AllocatorT>
	static void arch (ArchiveT& ar, std::vector<T, AllocatorT>& v, size_t sz, unsigned version) {
		for (size_t i=0; i<sz; ++i)
			ar | v[i];
	}
};
	
	
template<class ArchiveT, class T, class AllocatorT>
void archive (ArchiveT& ar, std::vector<T, AllocatorT>& v, unsigned version)
{
	u32 sz = (u32)v.size ();
	ar | sz;
	
	impl_archive_stdvector<
		std::is_fundamental<T>::value,
		ArchiveT::is_reading
	>::arch (ar, v, sz, version);
}
	
template<class ArchiveT, class Key, class T, class Compare, class Allocator>
inline void archive (ArchiveT& ar, std::map<Key, T, Compare, Allocator>& v, unsigned aver) {
	std_map_archive(ar, v, aver, std::integral_constant<bool, ArchiveT::is_reading>());
}
	
template<class ArchiveT, class Key, class T, class Compare, class Allocator>
void std_map_archive (ArchiveT& ar, std::map<Key, T, Compare, Allocator>& v, unsigned aver, pulmotor::true_t)
{
	typedef std::map<Key, T, Compare, Allocator> map_t;

	pulmotor::u32 sz = 0;
	ar | sz;
	while (sz--)
	{
		typename map_t::value_type value;
		ar | value;
		v.insert (v);
	}
}
	
template<class ArchiveT, class Key, class T, class Compare, class Allocator>
void std_map_archive (ArchiveT& ar, std::map<Key, T, Compare, Allocator>& v, unsigned aver, pulmotor::false_t)
{
	typedef std::map<Key, T, Compare, Allocator> map_t;
	
	ar | (pulmotor::u32)v.size();
	for (auto const& p : v)
		ar | p;
}

PULMOTOR_ARCHIVE_FREE_SPLIT(std::string)
PULMOTOR_ARCHIVE_FREE_READ(std::string)
{
	pulmotor::u32 size = 0;
	ar | size;
	v.resize (size);
	ar | pulmotor::memblock (v.c_str(), size);
}
PULMOTOR_ARCHIVE_FREE_WRITE(std::string)
{
	ar | (pulmotor::u32)v.size ();
	ar | pulmotor::memblock (v.c_str(), v.size ());
}

}

PULMOTOR_ARCHIVE_VER(std::string, pulmotor::version_dont_track);

#endif
