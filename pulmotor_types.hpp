#ifndef PULMOTOR_TYPES_HPP_
#define PULMOTOR_TYPES_HPP_

#include <string>

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
	
	enum { header_size = 8 };

	class blit_section;
	
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

	class access
	{
	public:
		template<class ArchiveT, class T>
		static void call_archive (ArchiveT& ar, T const& obj, unsigned version)
		{
			const_cast<T&> (obj).archive (ar, version);
		}

		template<class T>
		static void call_member (blit_section& ar, T const& obj, unsigned version)
		{
			const_cast<T&> (obj).serialize (ar, version);
		}
	};
	
	template<bool Condition, class T>
	struct enable_if {};
	
	template<class T>
	struct enable_if<true, T>
	{
		typedef T type;
	};

	template<class T> struct version {
		static int const value = 0;
	};
	template<class T> struct track_version {
		static bool const value = true;
	};

	template<class T>
	struct memblock_t
	{
		memblock_t (T const* p, size_t cnt) : addr ((uintptr_t)p), count (cnt) {}
		uintptr_t	addr;
		size_t		count;
	};
	template<class T>
	memblock_t<T> inline memblock (T const* p, size_t cnt) { return memblock_t<T> (p, cnt); }

	template<class T>
	struct ptr_address
	{
		ptr_address (T* const* p, size_t cnt) : addr ((uintptr_t)p), count (cnt) {}
		ptr_address (T const* const* p, size_t cnt) : addr ((uintptr_t)p), count (cnt) {}
		ptr_address (T const** p, size_t cnt) : addr ((uintptr_t)p), count (cnt) {}

		uintptr_t	addr;
		size_t		count;
	};

	template<class T, int N>
	struct array_address
	{
		array_address (T (*p)[N]) : addr ((uintptr_t)p) {}
		array_address (T const (*p)[N]) : addr ((uintptr_t)p) {}

		uintptr_t	addr;
	};
	
	struct blit_section_info
	{
		int 	data_offset;
		int		fixup_offset, fixup_count;
		int		reserved;
	};	
	
	namespace util
	{
		inline pulmotor::blit_section_info* get_bsi (void* data, bool dataIncludesHeader = true)
		{
			return (pulmotor::blit_section_info*) ((u8*)data + (dataIncludesHeader ? header_size : 0));
		}
		
		inline u8* get_root_data (void* data)
		{
			return (u8*)data + get_bsi (data, true)->data_offset + header_size;
		}
	}
	
	template<class T>
	struct data_ptr
	{
		typedef T value_type;
		
		data_ptr () : bsi_ (0)
		{
#ifdef _DEBUG
			debug_ptr_ = 0;
#endif
		}
		
		data_ptr (void* pulM_ptr) : bsi_ (util::get_bsi(pulM_ptr))
		{
#ifdef _DEBUG
			debug_ptr_ = (T*)((u8*)bsi_ + bsi_->data_offset);
#endif
		}
		
		T* get () const { return (T*)((u8*)bsi_ + bsi_->data_offset); }
		T* operator->() const { return (T*)((u8*)bsi_ + bsi_->data_offset); }
		T& operator*() const { return *(T*)((u8*)bsi_ + bsi_->data_offset); }
		
		void* start () const { return (u8*)bsi_ - header_size; };
		blit_section_info* bsi () const { return bsi_; };
		
	private:
		blit_section_info*	bsi_;
#ifdef _DEBUG
		T*	debug_ptr_;
#endif
	};
}

#endif // PULMOTOR_TYPES_HPP_
