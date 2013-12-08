#ifndef PULMOTOR_TYPES_HPP_
#define PULMOTOR_TYPES_HPP_

#include <string>
#include <type_traits>


#define PULMOTOR_ARCHIVE() template<class ArchiveT> void archive (ArchiveT& ar, unsigned pversion)
#define PULMOTOR_ARCHIVE_SPLIT() template<class ArchiveT> void archive (ArchiveT& ar, unsigned pversion) {\
typedef std::integral_constant<bool, ArchiveT::is_reading> is_reading_t; \
archive_impl(ar, pversion, is_reading_t()); }
#define PULMOTOR_ARCHIVE_READ() template<class ArchiveT> void archive_impl (ArchiveT& ar, unsigned pversion, pulmotor::true_t)
#define PULMOTOR_ARCHIVE_WRITE() template<class ArchiveT> void archive_impl (ArchiveT& ar, unsigned pversion, pulmotor::false_t)


#define PULMOTOR_ARCHIVE_FREE(T) template<class ArchiveT> void archive (ArchiveT& ar, T& v, unsigned version)
#define PULMOTOR_ARCHIVE_FREE_SPLIT(T) template<class ArchiveT> inline void archive (ArchiveT& ar, T& v, unsigned pversion) {\
typedef std::integral_constant<bool, ArchiveT::is_reading> is_reading_t; \
archive_impl(ar, v, pversion, is_reading_t()); }

#define PULMOTOR_ARCHIVE_FREE_READ(T) template<class ArchiveT> inline void archive_impl (ArchiveT& ar, T& v, unsigned pversion, pulmotor::true_t)
#define PULMOTOR_ARCHIVE_FREE_WRITE(T) template<class ArchiveT> inline void archive_impl (ArchiveT& ar, T& v, unsigned pversion, pulmotor::false_t)

// Construct and SaveConstruct helper macros

#define PULMOTOR_ARCHIVE_USE_CONSTRUCT() \
	template<class ArchiveT> \
	void archive (ArchiveT& ar, unsigned version) { \
		typedef std::integral_constant<bool, ArchiveT::is_reading> is_reading_t; \
		archive_use_construct_impl (ar, this, is_reading_t(), version); \
	} \
	template<class ArchiveT, class ObjT> \
	static void archive_use_construct_impl (ArchiveT& ar, ObjT* p, pulmotor::true_t, unsigned version) { \
		select_archive_construct(ar, p, version); \
	} \
	template<class ArchiveT, class ObjT> \
	static void archive_use_construct_impl (ArchiveT& ar, ObjT* p, pulmotor::false_t,  unsigned version) { \
		select_archive_save_construct(ar, p, version); \
	}


#define PULMOTOR_ARCHIVE_CONSTRUCT(xx) \
	template<class ArchiveT> static void archive_construct (ArchiveT& ar, xx, unsigned version)

#define PULMOTOR_ARCHIVE_SAVE_CONSTRUCT(xx) \
	template<class ArchiveT> void archive_save_construct (ArchiveT& ar, unsigned version)

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
	
	typedef std::true_type true_t;
	typedef std::false_type false_t;
	
#ifdef _WIN32
	typedef wchar_t pp_char;
	typedef std::wstring string;
#else
	typedef char pp_char;
	typedef std::string string;
#endif
	
	typedef u64 file_size_t;
	
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

		template<class ArchiveT, class T>
		static void call_serialize (ArchiveT& ar, T const& obj, unsigned version)
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
	
#define PULMOTOR_ARCHIVE_VER(T, v) template<> struct version<T> { enum { value = v }; }
	
#define PULMOTOR_ARCHIVE_NOVER(T) template<> struct track_version<T> { enum { value = 0 }; }
#define PULMOTOR_ARCHIVE_NOVER_TEMPLATE1(T) template<class TT> struct track_version< T < TT > > { enum { value = 0 }; }
#define PULMOTOR_ARCHIVE_NOVER_TEMPLATE2(T) template<class TT1, class TT2> struct track_version< T < TT1, TT2 > > { enum { value = 0 }; }
	

	template<class T>
	struct memblock_t
	{
		memblock_t (T const* p, size_t cnt) : addr ((uintptr_t)p), count (cnt) {}
		uintptr_t	addr;
		size_t		count;
	};
	template<class T>
	memblock_t<T> inline memblock (T const* p, size_t cnt) { return memblock_t<T> (p, cnt); }
	
	
	template<class AsT, class ActualT>
	struct as_holder
	{
		ActualT& m_actual;
		explicit as_holder (ActualT& act) : m_actual(act) {}
	};
	
	template<class AsT, class ActualT>
	inline as_holder<AsT, ActualT> as (ActualT& act)
	{
		return as_holder<AsT, ActualT> (act);
	}
	

	template<class T>
	struct ptr_address
	{
		ptr_address (T* const* p, size_t cnt) : addr ((uintptr_t)p), count (cnt) {}
		ptr_address (T const* const* p, size_t cnt) : addr ((uintptr_t)p), count (cnt) {}
		ptr_address (T const** p, size_t cnt) : addr ((uintptr_t)p), count (cnt) {}
		
		T*& ptrref() { return *(T**)addr; }

		uintptr_t	addr;
		size_t		count;
	};
	
	template<class T>
	struct archive_array
	{
		T* ptr;
		size_t count;
		
		archive_array (T* p, size_t cnt) : ptr(p), count(cnt) {}
		
		PULMOTOR_ARCHIVE_SPLIT()
		PULMOTOR_ARCHIVE_READ()
		{
			for (size_t i=0; i<count; ++i)
				load_construct (ar, &ptr[i], pversion);
		}
		PULMOTOR_ARCHIVE_WRITE()
		{
			for (size_t i=0; i<count; ++i)
				save_construct (ar, &ptr[i], pversion);
		}
		
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
		pulmotor::u32 	data_offset;
		pulmotor::u32	fixup_offset, fixup_count;
		pulmotor::u32	reserved;
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
		
		bool valid () const { return bsi_ != nullptr; }
		
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
