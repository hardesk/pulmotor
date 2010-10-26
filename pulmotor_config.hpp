#ifndef PULMOTOR_HPP_
#define PULMOTOR_HPP_

//#include "pulmotor_types.hpp"

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

#define PULMOTOR_STIR_PATH_SUPPORT 1

#endif
