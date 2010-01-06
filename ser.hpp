#ifndef STIR_SER_HPP_
#define STIR_SER_HPP_

#include <boost/tr1/type_traits.hpp>
#include <boost/type_traits/extent.hpp>
#include <boost/type_traits/is_class.hpp>
#include <boost/type_traits/is_integral.hpp>
#include <boost/type_traits/is_floating_point.hpp>

#define BOOST_MULTI_INDEX_DISABLE_SERIALIZATION 1
#include <boost/multi_index_container.hpp>
//#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/ordered_index.hpp>

#include <cstdarg>
#include <cstddef>
#include <cassert>
#include <vector>
#include <algorithm>
#include <list>
#include <set>
#include <map>
#include <typeinfo>
#include <string>

#include "stream_fwd.hpp"
#include "stream.hpp"

#define PULMOTOR_ADD_MARKERS 0

#define	pulmotor_addressof(x) (&reinterpret_cast<unsigned char const&> (x))

namespace pulmotor {

	enum member_type {
		k_integral,
		k_pointer,
		k_floating_point,
		k_other
	};

inline char const* to_string (member_type type) {
	switch (type) {
		case k_integral: return "integral"; 
		case k_pointer: return "pointer";
		case k_floating_point: return "floating point";
		default:
		case k_other: return "other";
	}
}

using namespace boost::multi_index;
using namespace std::tr1;

typedef unsigned char		u8;
typedef signed char			s8;
typedef unsigned short int	u16;
typedef signed short int	s16;
typedef unsigned int		u32;
typedef signed int			s32;

inline void set_offsets (void* data, std::pair<uintptr_t,uintptr_t> const* fixups, size_t fixup_count, bool change_endianess);
inline void fixup_pointers (void* data, uintptr_t const* fixups, size_t fixup_count);

template<int Size>
struct swap_element_endian;

template <>
struct swap_element_endian<2>
{
	static void swap (char* p) {
		u16 a = *(u16*)p;
		u32 b = ((a & 0x00ff) << 8) | ((a & 0xff00) >> 8);
		*(u16*)p = b;
	}
};

template <>
struct swap_element_endian<4>
{
	static void swap (char* p) {
		u32 a = *(u32*)p;
		u32 b = ((a & 0x000000ff) << 24) | ((a & 0x0000ff00) << 8) | ((a & 0x00ff0000) >> 8) | ((a & 0xff000000) >> 24);
		*(u32*)p = b;
	}
};

template<int Size>
void swap_endian (void* arg, size_t count)
{
	for (char* p = (char*)arg, *end = (char*)arg + count * Size; p != end; ++p)
	{
		swap_element_endian<Size>::swap (p);
	}
}

inline std::string shorten_name (char const* str)
{
	std::string res;
	size_t sz = strlen (str);
	res.reserve (sz);
	int open = 0;

	for (size_t i=0;i<sz; ++i)
	{
		if (str[i] == '<')
			open++;
		
		if (!open)
			res += str[i];

		if (str[i] == '>')
			open--;
	}
	return res;
}

inline void logf (char const* fmt, ...) {
	va_list vl;
	va_start (vl,fmt);
	vprintf (fmt, vl);
}

inline void logf_ident (int diff, char const* fmt, ...) {
	static int ident_level = 0;
	ident_level = std::max (0, ident_level + diff);
	if (fmt != 0)
	{
		for (int i=0; i<ident_level; ++i)
			logf (".  ");

		va_list vl;
		va_start (vl,fmt);
		vprintf (fmt, vl);
	}
}

template<class T> struct version {
	static int const value = 0;
};
template<class T> struct track_version {
	static bool const value = false;
};

namespace flag_util {

enum {
	k_member = 0x80000000,
};

template<int Flag>
inline bool is_flag (unsigned flags) {
	return (flags & Flag) != 0;
}	

template<int Flag>
inline unsigned set_flag (unsigned flags) {
	return flags | Flag;
}	
	
inline bool is_member (unsigned flags) {
	return (flags & 0x80000000) != 0;
}

inline unsigned get_version (unsigned flags) {
	return flags & 0x0000ffff;
}

}

struct primitive_tag {};
struct compound_tag {};
struct pointer_tag {};
struct array_tag {};
struct ptr_address_tag {};
struct array_address_tag {};
//struct exchange_tag {};

template<class T>
struct ref_wrapper {
	T* p;
	explicit ref_wrapper (T* t) : p(t) {} 
};

template<class T>
ref_wrapper<T> ref (T* t) {
	return ref_wrapper<T> (t);
}

template<class T>
struct ptr_address
{
	ptr_address (T* const* p, size_t cnt) : addr ((uintptr_t)p), count (cnt) {}
	ptr_address (T const* const* p, size_t cnt) : addr ((uintptr_t)p), count (cnt) {}

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

/*template<class Target, class Original>
struct exchange_t
{
	exchange_t (Target const& t, Original const& o)
	:	target (t), original (o) 
	{}
	
	Target		value;
	Original	original;
};*/

template<class T> inline ptr_address<T> ptr (T*& p, size_t cnt = 1) { return ptr_address<T> ((T**)pulmotor_addressof(p), cnt); }
template<class T> inline ptr_address<T> ptr (T const* const& p, size_t cnt = 1) { return ptr_address<T> ((T const* const*)pulmotor_addressof(p), cnt); }

//template<class T> inline ptr_address<T> ptr (T (*p)[], size_t cnt) { return ptr_address<T> (p, cnt); }
//template<class T> inline ptr_address<T> ptr (T const (*p)[], size_t cnt = 1) { return ptr_address<T> (p, cnt); }

template<class T, int N> inline array_address<T, N> array (T (*p)[N]) { return array_address<T, N> (p); }
template<class T, int N> inline array_address<T, N> array (T const (*p)[N]) { return array_address<T, N> (p); }

//template<class T, class O> inline exchange_t<T, O> exchange (T t, O o) { return exchange_t (t, o); }

template<class T>
struct is_ptr_address
	:	public boost::mpl::bool_<false>
{};

template<class T>
struct is_ptr_address<ptr_address<T> >
	:	public boost::mpl::bool_<true>
{};

template<class T>
struct is_array_address
	:	public boost::mpl::bool_<false>
{};

template<class T, int N>
struct is_array_address<array_address<T, N> >
	:	public boost::mpl::bool_<true>
{};

template<class T>
struct is_primitive :
	public boost::type_traits::ice_or<is_integral<T>::value, is_floating_point<T>::value>
{};

//template<class T>
//struct is_exchange : public boost::mpl::bool_<false> {}
//template<class T, class O>
//struct is_exchange<exchange_t<T, O> > : public boost::mpl::bool_<true> {}


template<class T>
struct strip_nonrelevant : public remove_cv<T>
{};

struct blit_section_info
{
	int		data_offset;
	int		fixup_offset, fixup_count;
};

template<class T>
struct get_member_type
{
	static const member_type value =
		is_integral<T>::value
			? k_integral
			: is_floating_point<T>::value
				? k_floating_point
				: is_pointer<T>::value
					? k_pointer
					: k_other
		;
};

class access
{
public:
	template<class ArchiveT, class T>
	static void call_member (ArchiveT& ar, T const& obj, unsigned version)
	{
		const_cast<T&> (obj).blit (ar, version);
	}
};

template<class T, class CreateDataT>
struct factory_creation
{
	factory_creation (void* ptr, CreateDataT const& data)
	:	id_ ((uintptr_t)ptr), data_ (data)
	{}

	uintptr_t	id_;	
	CreateDataT	data_;
};

template<class ArchiveT, class ObjectT>
inline void blit_compound (ArchiveT& ar, ObjectT& obj, unsigned version)
{
	blit (ar, obj, version);
}

template<class ArchiveT, class ObjectT>
inline void blit (ArchiveT& ar, ObjectT& obj, unsigned long version)
{
	access::call_member (static_cast<ArchiveT&> (ar), obj, version);
}

template<class ArchiveT, class ObjectT>
inline void blit_impl (ArchiveT& ar, ObjectT& obj, unsigned version, compound_tag)
{
	logf_ident (1, "by-value compound -> '%s', type: %s (v.%d)\n",
		typeid(ar).name(), shorten_name(typeid(obj).name()).c_str(), version);

	if (ar.member_mode_ == true)
		ar.register_member ( (uintptr_t)pulmotor_addressof (obj), sizeof (obj), 1, get_member_type<ObjectT>::value);

//	ar.begin_area (&obj, 1, version);
	blit_compound (ar, obj, version);
//	ar.end_area (&obj, version);
	logf_ident (-1, 0);
}

template<class ArchiveT, class ObjectT>
inline void blit_impl (ArchiveT& ar, ObjectT& obj, unsigned version, primitive_tag)
{
	logf_ident (1, "primitive -> '%s', type: %s (v.%d)\n",
		typeid(ar).name(), shorten_name(typeid(obj).name()).c_str(), version);

	if (ar.member_mode_ == true)
		ar.register_member ( (uintptr_t)pulmotor_addressof (obj), sizeof (obj), 1, get_member_type<ObjectT>::value);

	logf_ident (-1, 0);
}

// true, array of primitives
template<class ArchiveT, class ObjectT>
inline void blit_array_helper (ArchiveT& ar, ObjectT* obj, size_t array_size, unsigned version, boost::mpl::bool_<true>)
{
	// do nothing, or copy memory block
	logf_ident (1, "blit-array  (x%d) -> '%s', type: %s (v.%d)\n",
		array_size, typeid(ar).name(), shorten_name(typeid(obj).name()).c_str(), version);

	if (ar.member_mode_ == true)
		ar.register_member ( (uintptr_t)pulmotor_addressof (obj), sizeof (*obj), array_size, get_member_type<ObjectT>::value);

	logf_ident (-1, 0);
}

// false, array of classes or pointers
template<class ArchiveT, class ObjectT>
inline void blit_array_helper (ArchiveT& ar, ObjectT* obj, size_t array_size, unsigned version, boost::mpl::bool_<false>)
{
	logf_ident (1, "blit-array (x%d) -> '%s', type: %s (v.%d)\n",
		array_size, typeid(ar).name(), shorten_name(typeid(*obj).name()).c_str(), version);
	for (size_t i=0; i<array_size; ++i)
	{
		blit (ar, obj [i]);
	}
	logf_ident (-1, 0);
}

template<class ArchiveT, class ObjectT>
inline void blit_impl (ArchiveT& ar, ObjectT& obj, unsigned version, array_tag)
{
	ObjectT* aa = (ObjectT*)pulmotor_addressof (obj);

	logf_ident (1, "array (@ %p) -> '%s', type: %s (v.%d)\n",
		aa, typeid(ar).name(), shorten_name(typeid(obj).name()).c_str(), version);

	typedef typename remove_all_extents<typename remove_pointer<ObjectT>::type>::type pointee_t;
	logf ("  >>>> pointee: %s\n", typeid(pointee_t).name ());
	size_t const array_size = boost::extent<ObjectT>::value;

	bool mm = ar.member_mode_;
	ar.begin_area (aa, array_size, version, false);
	blit_array_helper (ar, *aa, array_size, version,
		boost::mpl::bool_<is_primitive<pointee_t>::value> ()
	);
	ar.end_area (aa, version, mm);

	logf_ident (-1, 0);
}

template<class ArchiveT, class ObjectT>
inline void blit_impl (ArchiveT& ar, ptr_address<ObjectT> objaddr, unsigned version, ptr_address_tag)
{
	ObjectT** ppobj = reinterpret_cast<ObjectT**> (objaddr.addr);

	logf_ident (1, "ptr-addr (%p @ %p), x%d-> '%s', type: %s (v.%d)\n",
		*ppobj, objaddr.addr, objaddr.count, typeid(ar).name(), shorten_name(typeid(ObjectT).name()).c_str(), version);

	ar.register_member ( (uintptr_t)ppobj, sizeof (ObjectT*), 1, get_member_type<ObjectT*>::value);
//	ar.register_pointer_field (objaddr.addr);
//	ar.register_pointer (*ppobj);
	
	bool already_registered = ar.has_registered (*ppobj);

	if (already_registered)
		logf_ident (0, "Object/ptr already registered\n");

	typedef typename boost::mpl::bool_<is_primitive<ObjectT>::value>::type is_prim_t;

	// process class only if pointer is valid and not registered yet
	if (!already_registered && *ppobj)
	{
		bool mm = ar.member_mode_;
		ar.begin_area (*ppobj, objaddr.count, version, false);
		blit_array_helper (ar, *ppobj, objaddr.count, version, is_prim_t ());
		ar.end_area (*ppobj, version, mm);
	}

	logf_ident (-1, 0);
}

template<class ArchiveT, class ObjectT>
inline void blit_impl (ArchiveT& ar, ObjectT& obj, unsigned version, pointer_tag)
{
	ObjectT* po = (ObjectT*)pulmotor_addressof(obj);
	ptr_address<typename remove_pointer<ObjectT>::type> adr(po, 1);
	blit_impl (ar, adr, version, ptr_address_tag ());
}

// examine obj and forward serialization to the appropriate function by selecting the right tag
template<class ArchiveT, class ObjectT>
inline void blit_redirect (ArchiveT& ar, ObjectT& obj, unsigned flags_version)
{
	using boost::mpl::identity;
	using boost::mpl::eval_if;

	typedef typename remove_cv<ObjectT>::type actual_t;
	typedef typename boost::mpl::eval_if<
		is_primitive<actual_t>,
			identity<primitive_tag>,
			eval_if<
				is_array<actual_t>,
				identity<array_tag>,
				eval_if<
					is_ptr_address<actual_t>,
					identity<ptr_address_tag>,
					eval_if<
						is_array_address<actual_t>,
						identity<array_address_tag>,
						eval_if<
							is_pointer<actual_t>,
							identity<pointer_tag>,
							identity<compound_tag>
						>
					>
				>
			>
	>::type tag_t;
	blit_impl (ar, obj, flags_version, tag_t());
}

// entry point
template<class ArchiveT, class ObjectT>
inline ArchiveT& blit_object (ArchiveT& ar, ObjectT& obj)
{
	unsigned const ver = version<typename strip_nonrelevant<ObjectT>::type>::value;
	ar.begin_area (&obj, 1, ver, false);
	blit_redirect (ar, obj, ver);
	ar.end_area (&obj, ver, false);
	return ar;
}

template<class ArchiveT, class ObjectT>
inline ArchiveT& blit (ArchiveT& ar, ObjectT& obj)
{
	unsigned const ver = version<typename strip_nonrelevant<ObjectT>::type>::value;
	blit_redirect (ar, obj, ver);
	return ar;
}

template<class ArchiveT, class ObjectT>
inline ArchiveT& operator & (ArchiveT& ar, ObjectT const& obj)
{
	return blit (ar, const_cast<ObjectT&>(obj));
}

template<class ArchiveT, class ObjectT>
inline ArchiveT& operator | (ArchiveT& ar, ObjectT const& obj)
{
	return blit (ar, const_cast<ObjectT&>(obj));
}

struct blit_section
{
	enum reftype {
		k_object,
		k_array
	};

	enum objtype {
		k_free = 0, // object is helf by pointer
		k_compounded = 1, // object is part of an array or is a by-value member
	};
	
	enum {
		archive_is_loading = false,
		archive_is_saving = true
	};

	struct member_info
	{
		member_info (size_t offset, unsigned size, size_t count, member_type type)
			:	offset_(offset), count_ (count), size_ (size), type_ (type)
	   	{}
	   	
	   	bool operator== (member_info const& a) const {
	   		return a.offset_ == offset_ && a.count_ == count_ && a.size_ == size_ && a.type_ == type_;
	   	}
		size_t	offset_;
		size_t	count_;
		u16		size_;
		u16		type_;
	};

	struct ref
	{
		ref (size_t offset)
			:	offset_(offset)
		{}

		bool operator==(ref const& a) const { return offset_ == a.offset_; }
		size_t		offset_;
	};
	
	struct class_info
	{
		typedef std::vector<member_info> member_container;

		class_info (std::type_info const* ti, size_t size, size_t align)
		:	ti_(ti), size_(size), align_(align), complete_(false)
		{}

		char const* get_name () const
		{	return ti_->name (); }

		bool is_complete () const
		{	return complete_; }
		void set_complete (bool complete)
		{	complete_ = complete; }

		void add_member (size_t offset, size_t size, size_t count, member_type type)
		{
			// TODO: what to do with unions?
			// check for duplication (this will happen if a class has a pointer to the same type of object). eg. class S { S* ps; };
			member_info mi (offset, size, count, type);
			member_container::iterator it = std::find (members_.begin (), members_.end (), mi);
			if (it == members_.end ()) {
				logf_ident (1, "member. offset: %lu, size: %lu, count: %lu, type: %s\n", offset, size, count, to_string(type));
				members_.push_back (mi);
			}
			logf_ident (-1, 0);
		}

		void dump_members ()
		{
			logf ("class '%s', %lu members\n", get_name (), members_.size ());
			for (size_t i=0; i<members_.size(); ++i)
			{
				member_info const& mi = members_[i];
				logf ("  member [%2lu]: offset:%3lu, size:%2d, count:%2lu, type:%s\n", i, mi.offset_, mi.size_, mi.count_, to_string((member_type)mi.type_));
			}
		}

		void change_endianess (void* ptr, size_t size)
		{
			for (size_t q=0; q<members_.size(); ++q)
			{
				member_info const& mi = members_[q];
				char* p = (char*)ptr + mi.offset_;
				switch (mi.size_)
				{
					default:
					case 1:
						break;

					case 2:
						swap_endian<2> (p, mi.count_);
						break;
					case 4:
						swap_endian<4> (p, mi.count_);
						break;
//					case 8:
//						swap_endian<8> (p, mi.count_);
//						break;
				}
			}
		}
		
		typedef member_container::const_iterator member_iterator;
		member_iterator member_begin () const { return members_.begin (); }
		member_iterator member_end () const { return members_.end (); }
		
		size_t get_size () const
		{	return size_; }
		size_t get_align () const
		{	return align_; }

		std::type_info const*	ti_;
	private:
		size_t					size_, align_;
		bool					complete_;
		member_container		members_;
	};

	struct object
	{
		object (class_info*	ci, reftype rt, objtype ot, void const* data, size_t size)
			:	ci_(ci), data_((uintptr_t)data), size_(size), reftype_(rt), objtype_(ot), file_offset_(-1)
		{}

		class_info*			ci_;

		uintptr_t			data_;
		size_t				size_; // in bytes

		reftype get_reftype () const { return reftype_; }
		void set_reftype (reftype reft) { reftype_ = reft; }

		objtype get_objtype () const { return objtype_; }
		void set_objtype (objtype objt) { objtype_ = objt; }

		// can be raw memory with pointers and therefore do not have class_info
		bool is_class () const { return ci_ != 0; }
		
		bool has_file_offset () const { return file_offset_ != (uintptr_t)-1; }
		void set_file_offset (uintptr_t fo) { file_offset_ = fo; }
		uintptr_t file_offset () const { return file_offset_; }

		class_info* get_class_info () const { return ci_; }
		
	private:
		reftype			reftype_;
		objtype			objtype_;
		uintptr_t		file_offset_;
	};
	
	struct objectptr_less_addr {
		bool operator()(object const* a, object const* b) const {
			return a->data_ < b->data_;
		}
		bool operator()(uintptr_t a, object const* b) const {
			return a < b->data_;
		}
		bool operator()(object const* a, uintptr_t b) const {
			return a->data_ < b;
		}
	};

	struct select_first {
		template<class T1, class T2>
		T1 operator () (std::pair<T1, T2> const& a) const { return a.first; }
	};
	
	typedef std::vector<object*> object_container_t;
	typedef std::map<void*, object*> object_map_t;
	
	struct ptr_tag {};
	typedef multi_index_container<
		object*,
		indexed_by<
			ordered_non_unique<tag<ptr_tag>, identity<object*>, objectptr_less_addr>
		>
	> object_mindex_t;
	
	typedef object_mindex_t::index<ptr_tag>::type	object_ptr_t;
	
	object_mindex_t		objects_;
	object_container_t	parent_stack_;
	object*				first_object_;
	bool				member_mode_; // true if serializing members, false if free object (or similar)
	
	typedef std::map<std::type_info const*, class_info*> class_info_container_t;
	class_info_container_t class_infos_;
	
	blit_section ()
	:	first_object_ (0)
	,	member_mode_ (false)
	{}
	
	template<class T>
	class_info* get_class_info ()
	{
		class_info_container_t::iterator it = class_infos_.find (&typeid(T));
		if (it == class_infos_.end ()) {
			it = class_infos_.insert (
				std::make_pair (
					&typeid(T),
					new class_info(&typeid(T), sizeof(T), 4))
			).first;
		}
		return it->second;
	}

	void register_member (uintptr_t ptr, size_t size, size_t count, member_type type)
	{
		assert (!parent_stack_.empty ());
		object* po = parent_stack_.back ();
		if (po->is_class () && !po->get_class_info ()->is_complete ())
		{
			uintptr_t offset = ptr - po->data_;
			bool belongs = ptr >= po->data_ && ptr < po->data_ + po->size_;
			if (!belongs) {
				logf ("ERROR: member (ptr:%p, size:%lu, count:%lu, type:%s) does not belong"
					   " to parent (ptr: %p, size:%lu, type:%s)\n",
						ptr, size, count, to_string (type),
						po->data_, po->size_, po->is_class () ? po->ci_->get_name () : "<basic>");
			}
			assert ((ptr >= po->data_ && ptr < po->data_ + po->size_) && "Member does not belong to the parent object");
			assert (offset < po->size_ && "Offset computed is too large");
			po->get_class_info ()->add_member (offset, size, count, type);
		}
	}

	// pass a pointer to an object to check if that object is already registered. pointer passed can point to
	// the middle of an object.
	bool has_registered (void* p)
	{
		object_ptr_t::iterator it = objects_.get<ptr_tag> ().lower_bound ((uintptr_t)p, objectptr_less_addr());
		if (it != objects_.get<ptr_tag> ().end ()) {
			bool belongs = (uintptr_t)p - (*it)->data_ < (*it)->size_;
			return belongs;
		}
		return false;
	}

	template<class T>
	void begin_area (T const* obj, size_t count, unsigned version, bool childCompoundExpectingMember)
	{
		uintptr_t op = (uintptr_t)obj;

		member_mode_ = childCompoundExpectingMember;
		
		objtype ot = k_free;

		logf_ident (0, "[object %p, %d\n", obj, count);

		// check if this object belongs to the array (parent), and if it does, mark this obj as compounded
		if (!parent_stack_.empty ()) {
			object* po = parent_stack_.back ();
			if (op >= po->data_ && op <= po->data_ + po->size_) {
				// this object is either a member of its parent or a member of an array
				ot = k_compounded;
			}
		}

		class_info* ci = get_class_info<T> ();

		object* oi = new object (ci, count == 1 ? k_object : k_array, ot, obj, sizeof(T) * count);
		if (!first_object_)
			first_object_ = oi;
		objects_.get<ptr_tag> ().insert (oi);
		parent_stack_.push_back (oi);
	}

	template<class T>
	void end_area (T const* obj, unsigned version, bool previousMemberMode)
	{
		class_info* ci = get_class_info<T> ();
		if (!ci->is_complete()) {
			ci->set_complete (true);
		}
		
		member_mode_ = previousMemberMode;

		parent_stack_.pop_back ();

		logf_ident (0, "]object %p\n", obj);
	}

	void dump_gathered ()
	{
		logf ("\n ------------------- classes\n");
		for (class_info_container_t::iterator it = class_infos_.begin (), end = class_infos_.end (); it != end; ++it)
		{
			logf ("class (%p): '%s', size: %d (%d)\n", it->second,
				shorten_name(it->second->ti_->name ()).c_str(), it->second->get_size (), it->second->get_align());
		}

		logf ("\n ------------------- pointer objects\n");

		int i = 0;
		for (object_ptr_t::iterator it = objects_.get<ptr_tag>().begin (), end = objects_.get<ptr_tag>().end (); it != end; ++it)
		{
			object const* o = *it;
			logf ("  [%03d] %s: %p, objtype:%s, size:%d class: %p '%s'\n",
				i, o->get_reftype () == k_object ? "obj" : "arr", o->data_,
				o->get_objtype () == k_free ? "free" : "compounded",
				o->size_, o->ci_, o->get_class_info () ? shorten_name (o->get_class_info ()->get_name ()).c_str () : "raw");

			++i;
		}
	}

	typedef std::vector<std::pair<uintptr_t, uintptr_t> > ptr_refs_t;
		
	void append (std::vector<unsigned char>& buffer, void const* data, size_t size)
	{
		buffer.insert (buffer.end (), (unsigned char const*)data, (unsigned char const*)data + size);
	}

	void write_out (std::vector<unsigned char>& output_buffer, bool change_endianess)
	{
//		std::sort (objects_.begin (), objects_.end (), objectptr_less_addr());

		size_t initialBufferSize = output_buffer.size ();

		// offsets to 'pointers' in the written stream
		ptr_refs_t pr;
	
		std::vector<unsigned char> data_buffer;
		if (first_object_)
			write_out (first_object_, data_buffer, pr, change_endianess);

		std::vector<uintptr_t> fixups;
		std::transform (pr.begin (), pr.end (), std::back_inserter(fixups), select_first ());
	
		std::sort (fixups.begin (), fixups.end ());

		for (size_t i=0; i<fixups.size (); ++i) {
			logf ("fixup [%2d] at: 0x%08x\n", i, fixups[i]);
		}

#if PULMOTOR_ADD_MARKERS
		size_t const marker_size = 16;
		char mark2 [marker_size+1] = "-----fixups-----";
#endif

		pulmotor::basic_header hdr (0x00000001, "test_archive", 0);

		blit_section_info bsi;
		bsi.data_offset = sizeof bsi;
		bsi.fixup_offset= data_buffer.size () + bsi.data_offset
#if PULMOTOR_ADD_MARKERS
			+ marker_size
#endif
			;
		bsi.fixup_count	= fixups.size ();

		append (output_buffer, &hdr, sizeof hdr);
		append (output_buffer, &bsi, sizeof bsi);
//		logf ("blit_section_info: %d bytes\n", sizeof bsi);

		if (!pr.empty())
			set_offsets (&*data_buffer.begin (), &*pr.begin (), pr.size (), change_endianess);
		logf ("data: %d bytes\n", data_buffer.size());

		if (!data_buffer.empty())
			append (output_buffer, &*data_buffer.begin (), data_buffer.size ());

#if PULMOTOR_ADD_MARKERS
		append (output_buffer, (unsigned char*)mark2, 16); 
#endif

		if (!fixups.empty())
			append (output_buffer, &*fixups.begin (), fixups.size () * sizeof (uintptr_t));
			
		logf ("total: %d bytes\n", output_buffer.size () - initialBufferSize);
	}

	uintptr_t write_out (object* o, std::vector<unsigned char>& buffer, ptr_refs_t& pr, bool change_endianess)
	{
		if (o->has_file_offset ())
			return o->file_offset ();

		logf_ident (1, "obj: %p, size: %d, is-class: %d, type: %s\n", o->data_, o->size_, o->is_class (), o->is_class () ? o->get_class_info ()->get_name () : "<unvailable>");

		// todo: align buffer

		// offset of this object in the output buffer (file)
		uintptr_t thisOffset = buffer.size ();
		o->set_file_offset (thisOffset);

		// insert the data of the object into the buffer
		buffer.insert (buffer.end (), (char*)o->data_, (char*)o->data_ + o->size_);
		
		// pointer to the copied data of this object
		u8 const* odat = &*(buffer.begin () + thisOffset);
		logf_ident (0, "--> file at %p (buffer-addr: %p)\n", thisOffset, odat);
		

//		if (class_info* ci = o->get_class_info ())
//			ci->dump_members ();

		class_info* ci = o->get_class_info ();

		// walk through members of this class, patch the pointers (convert memory-address to file offset) and follow pointers
		for (class_info::member_iterator it = ci->member_begin (), end = ci->member_end (); it != end; ++it)
		{
			// fetch pointer to this object data again, as buffer may be changed by child's write_out
			odat = &*(buffer.begin () + thisOffset);
			logf_ident (0, "member-%02d: %s, size: %d\n", std::distance (ci->member_begin (), it), to_string ((member_type)it->type_), it->size_); 

			if (it->type_ != k_pointer)
				continue;

			// address of the pointer in the already copied data
			uintptr_t const* ptrAddr = (uintptr_t const*) (odat + it->offset_);
			uintptr_t ptr = *ptrAddr;

			logf_ident (0, "ptr: %p @%p\n", ptr, ptrAddr);

			// if pointer to object is null, do nothing
			if (ptr == 0) {
				logf_ident (0, "Pointer to object is null, skipping\n");
				continue;
			}
			
			// find an object this pointer is referencing to
			object_ptr_t::iterator refIt = objects_.get<ptr_tag>().lower_bound (ptr, objectptr_less_addr ());
			
			if (refIt == objects_.get<ptr_tag>().end ())
			{
				// TODO: do something
				assert ("object reference was not found in the database");
			}
			
			// referenced object
			object& refObj = **refIt;

			// make sure that the pointer actually points to the object (or to null)
			if (!(refObj.data_ >= ptr && ptr - refObj.data_ < refObj.size_))
			{
				logf ("ERROR: object %p (ptr to it at %p). Registered data pointer: %p, size: %d\n", ptr, ptrAddr, refObj.data_, refObj.size_);
			}

			assert ((refObj.size_ == 0 || (refObj.data_ >= ptr && ptr - refObj.data_ < refObj.size_)) && "Memory range declared by the referenced object does not encompass the reference");
			
			// reference may point into the middle of the object, remeber the offset
			int offsetInReferencee = ptr - refObj.data_;
			
			// pointer location in the stream (of already written object)
			uintptr_t ptrLocationOffset = it->offset_ + thisOffset;
			
			// offset of the object that 'this' reference is pointing to
			uintptr_t referenceeOffset = write_out (&refObj, buffer, pr, change_endianess);
			
			pr.push_back (std::make_pair (ptrLocationOffset, referenceeOffset + offsetInReferencee));
		}
			
		if (change_endianess && o->is_class ())
		{
			if (class_info* ci = o->get_class_info ())
			{
				ci->dump_members ();
				ci->change_endianess (&*(buffer.begin () + thisOffset), o->size_);
			}
		}

		logf_ident (-1, 0);
		
		return thisOffset;
	}
};

inline void set_offsets (void* data, std::pair<uintptr_t,uintptr_t> const* fixups, size_t fixup_count, bool change_endianess)
{
	char* datap = (char*)data;
	for (size_t i=0; i<fixup_count; ++i)
	{
		uintptr_t* p = reinterpret_cast<uintptr_t*> (datap + fixups[i].first);
		*p = (uintptr_t)fixups[i].second;
		if (change_endianess)
			swap_endian<sizeof(*p)> (p, 1); 
	}
}

inline void fixup_pointers (void* data, uintptr_t const* fixups, size_t fixup_count)
{
	char* datap = (char*)data;
	for (size_t i=0; i<fixup_count; ++i)
	{
		uintptr_t* p = reinterpret_cast<uintptr_t*> (datap + fixups[i]);
		*p += (uintptr_t)data;
	}
}

namespace util
{

inline pulmotor::blit_section_info* get_bsi (void* data, bool skipHeader)
{
	return (pulmotor::blit_section_info*) ((char*)data + (skipHeader ? 0 : sizeof (pulmotor::basic_header)));
}

template<class T>
inline void blit_to_container (T& a, std::vector<unsigned char>& odata, bool change_endianess)
{
	pulmotor::blit_section bs;
	pulmotor::blit_object (bs, a);
	bs.write_out (odata, change_endianess);
}

template<class T>
inline T* fixup_pointers (pulmotor::blit_section_info* bsi)
{
	char* data = ((char*)bsi + bsi->data_offset);
	uintptr_t* fixups = (uintptr_t*)((char*)bsi + bsi->fixup_offset);

	pulmotor::fixup_pointers (data, fixups, bsi->fixup_count);

	T* pt = (T*)data;
	return pt;
}

}

} // pulmotor

#endif
