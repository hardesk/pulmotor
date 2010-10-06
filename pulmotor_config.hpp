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
typedef unsigned long long	u64;
typedef signed long long	s64;

#ifdef _WIN32
	typedef wchar_t pp_char;
	typedef std::wstring string;
#else
	typedef char pp_char;
	typedef std::string string;
#endif


	struct target_traits
	{
		// size of the pointer in bits
		size_t	ptr_size;
		bool	big_endian;

		static target_traits const current;
		
		static target_traits const le_lp32;
		static target_traits const be_lp32;
		
		static target_traits const le_lp64;
		static target_traits const be_lp64;
	};

	struct blit_section_info
	{
		int 	data_offset;
		int		fixup_offset, fixup_count;
		int		reserved;
	};	
}

#endif
