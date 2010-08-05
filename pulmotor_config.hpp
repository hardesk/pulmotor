#ifndef PULMOTOR_HPP_
#define PULMOTOR_HPP_

#ifdef _WIN32
#pragma warning (disable: 4275)
#	if PULMOTOR_DLL
#		if PULMOTOR_BUILD_DLL
#			define PULMOTOR_ATTR_DLL __declspec(dllexport)
//#pragma message ("PULMOTOR_ATTR_DLL __declspec(dllexport)")
#		else
#			define PULMOTOR_ATTR_DLL __declspec(dllimport)
//#pragma message ("PULMOTOR_ATTR_DLL __declspec(dllimport)")
#		endif
#	endif
#endif

#ifndef PULMOTOR_ATTR_DLL
#define PULMOTOR_ATTR_DLL
#endif

#include <string>

#define PULMOTOR_STIR_PATH_SUPPORT 1

namespace pulmotor
{

typedef unsigned char		u8;
typedef signed char			s8;
typedef unsigned short int	u16;
typedef signed short int	s16;
typedef unsigned int		u32;
typedef signed int			s32;

#ifdef _WIN32
	typedef wchar_t pp_char;
	typedef std::wstring string;
#else
	typedef char pp_char;
	typedef std::string string;
#endif
	
}

#endif
