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
	typedef wchar_t path_char;
	typedef std::wstring path_string;
#else
	typedef char path_char;
	typedef std::string path_string;
#endif
	
	typedef u64 fs_t;
	
	enum { header_size = 8 };
	enum : unsigned
	{
		version_dont_track = 0xffff'ffffu,
		no_version = 0xffff'ffffu,
		version_default = 0u,
		
		// u32 follows 'version' which specifies how much to advance stream (this u32 not included) to get to object data
		// in other words how much unknown information is between the version and object
		ver_flag_garbage_length = 0x80000000u,

		// arbitraty "debug" string is present after (optional garbage length)
		ver_flag_debug_string = 0x4000'0000u,
		ver_debug_string_max_size = 0x100u,

		// align object on an alignment address specified by archive
		ver_flag_align_object = 0x20000000u,

		// mask to get only the version part
		ver_flag_mask = 0x00ffffffu
   };
		

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
	
	template<bool Condition, class T>
	struct enable_if {};
	
	template<class T>
	struct enable_if<true, T>
	{
		typedef T type;
	};

	template<class T, class = void> struct has_version_member : std::false_type {};
	template<class T>               struct has_version_member<T, std::void_t<char [(std::is_convertible<decltype(T::version), unsigned>::value) ? 1 : -1]> > : std::true_type {};

	template<class T> struct class_version { static unsigned const value = version_default; };
	
	template<class T, bool HasMember = false>	struct get_version_impl				: std::integral_constant<unsigned, pulmotor::class_version<T>::value> {};
	template<class T>							struct get_version_impl<T, true>	: std::integral_constant<unsigned, T::value> {};
	
	template<class T> struct get_version : std::integral_constant<unsigned, get_version_impl<T>::value> {};
	
#define PULMOTOR_ARCHIVE_VER(T, v) template<> struct ::pulmotor::class_version<T> { enum { value = v }; }
	
	template<class T>
	struct clean_type
	{
		typedef typename std::remove_cv<T>::type type;
	};
	
	template<bool Constructed, class T>
	struct memblock_t
	{
		explicit memblock_t (T const* p, size_t cnt) : addr ((uintptr_t)p), count (cnt) {}
		
		T* ptr_at (size_t index) const { return (T*)addr + index; }
		
		uintptr_t	addr;
		size_t		count;
	};
	template<bool Constructed = true, class T>
	memblock_t<Constructed, T> inline memblock (T const* p, size_t cnt) { return memblock_t<Constructed, T> (p, cnt); }

	template<class T> struct is_memblock : public std::false_type {};
	template<class T, bool Constr> struct is_memblock<memblock_t<Constr, T> > : public std::true_type {};
	
	
	typedef char version_t;

	struct romu3
	{
		#define PULMOTOR_ROTL(d,lrot) ((d<<(lrot)) | (d>>(8*sizeof(d)-(lrot))))
		uint64_t xState=0xe2246698a74e50e0ULL, yState=0x178cd4541df4e31cULL, zState=0x704c7122f9cfbd76ULL;
		void seed(uint64_t a, uint64_t b,uint64_t c) { xState=a; yState=b; zState=c; }
		uint64_t operator() () {
		   uint64_t xp = xState, yp = yState, zp = zState;
		   xState = 15241094284759029579u * zp;
		   yState = yp - xp;  yState = PULMOTOR_ROTL(yState,12);
		   zState = zp - yp;  zState = PULMOTOR_ROTL(zState,44);
		   return xp;
		}
		unsigned r(unsigned range) { return uint64_t(unsigned(operator()())) * range >> (sizeof (unsigned)*8); }
	};
	
	template<class T>
	struct nv_impl
	{
		char const* name;
		T const& obj;
		
		nv_impl(char const* name_, T const& o) : name(name_), obj(o) {}
		nv_impl(nv_impl const& o) : name(o.name), obj(o.obj) {}
	};
	
	template<class T>
	inline nv_impl<T> make_nv(char const* name, T const& o)
	{
		return nv_impl<T> (name, o);
	}
	
#define nv1(x) ::pulmotor::make_nv(#x, x)
#define nv(x) nv1(x)
#define nv_(n,x) ::pulmotor::make_nv(n, x)
	
	template<class T> struct is_nvp : public std::false_type {};
	template<class T> struct is_nvp<nv_impl<T>> : public std::true_type {};
	
	
	template<class AsT, class ActualT>
	struct as_holder
	{
		ActualT& m_actual;
		explicit as_holder (ActualT& act) : m_actual(act) {}
	};
	
	template<class AsT, class ActualT>
	inline as_holder<AsT, ActualT> as (ActualT& act)
	{
		// Actual may be const qualified. We actually want that so that as works when writing, with const types
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
} // pulmotor

#endif // PULMOTOR_TYPES_HPP_
