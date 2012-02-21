#ifndef PULMOTOR_ARCHIVE_STD_HPP_
#define PULMOTOR_ARCHIVE_STD_HPP_

#include "archive.hpp"

namespace pulmotor
{

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
			v.push_back (e);
		}
	} else {
		for (typename std::list<T, AllocatorT>::iterator it = v.begin (), end = v.end (); it != end; ++it)
			ar | *it;
	}
}

template<class ArchiveT, class T, class AllocatorT>
void archive (ArchiveT& ar, std::vector<T, AllocatorT>& v, unsigned version)
{
	u32 sz = v.size ();
	ar | sz;

	if (ArchiveT::is_reading)
		v.resize (sz);

	if (std::tr1::is_fundamental<T>::value)
		ar & pulmotor::memblock (&*v.begin(), sz);
	else
		for (size_t i=0; i<sz; ++i)
			ar | v[i];
}

PULMOTOR_ARCHIVE_FREE_SPLIT(std::string)
PULMOTOR_ARCHIVE_FREE_READ(std::string)
{
	pulmotor::u32 size = 0;
	ar & size;
	v.resize (size);
	ar & pulmotor::memblock (v.c_str(), size);
}
PULMOTOR_ARCHIVE_FREE_WRITE(std::string)
{
	ar & (pulmotor::u32)v.size ();
	ar & pulmotor::memblock (v.c_str(), v.size ());
}

}

#endif
