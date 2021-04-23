#ifndef PULMOTOR_BLIT_HOLDER_HPP_
#define PULMOTOR_BLIT_HOLDER_HPP_

#include <cstddef>
#include "pulmotor_fwd.hpp"

#if PULMOTOR_STIR_PATH_SUPPORT
#include <stir/path.hpp>
#endif

namespace pulmotor {
	
namespace detail {

void load_impl (char const* fname, size_t& size, u8*& data);	

}
	
template<class T>
class blit_holder
{
	u8*		data_;
	size_t	size_;
#ifdef _DEBUG
	T const* object_;
#endif

	blit_holder (blit_holder const&);
	blit_holder& operator= (blit_holder const&);

public:
	blit_holder (pulmotor::pp_char const* fname)
	:	data_ (0)
	,	size_ (0)
#if _DEBUG
	,	object_ (0)
#endif
	{
		load (fname);
	}
	
#if PULMOTOR_STIR_PATH_SUPPORT
	explicit blit_holder (stir::path const& fname)
	:	data_ (0), size_ (0)
#if _DEBUG
	,	object_ (0)
#endif
	{
		load (fname.c_str());
	}
#endif
	
	blit_holder ()
	:	data_ (0)
	,	size_ (0)
	#if _DEBUG
	,	object_ (0)
	#endif
	{}

	void load (pulmotor::pp_char const* fname)
	{
		detail::load_impl (fname, size_, data_);		
	#if _DEBUG
		object_ = &ref ();
	#endif
	}
	
#if PULMOTOR_STIR_PATH_SUPPORT
	void load (stir::path const& fname)
	{ return load (fname.c_str()); }
#endif

	~blit_holder ()
	{
		if (data_)
			delete []data_;
	}

	bool good () const { return data_ != 0; }

	T const& ref () const { return *reinterpret_cast<T const*> (pulmotor::util::get_root_data (data_)); }	
	T const* operator-> () const { return reinterpret_cast<T const*> (pulmotor::util::get_root_data (data_)); }
};

}

#endif // PULMOTOR_BLIT_HOLDER_HPP_
