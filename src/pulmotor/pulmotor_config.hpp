#ifndef PULMOTOR_HPP_
#define PULMOTOR_HPP_

#if defined(_WIN32)
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

#if !defined(PULMOTOR_EXCEPTIONS)
#define PULMOTOR_EXCEPTIONS __cpp_exceptions
#endif

#if !defined(PULMOTOR_RTTI)
#define PULMOTOR_RTTI __cpp_rtti
#endif

#if defined(__MSC_VER)
#define PULMOTOR_NOINLINE __declspec(noinline)
#else
#define PULMOTOR_NOINLINE __attribute__((noinline))
#endif

#define PULMOTOR_UNUSED(x) ((void)x)
//#define PULMOTOR_STIR_PATH_SUPPORT 1

#endif
