#ifndef STIR_SER_HPP_
#define STIR_SER_HPP_

#include <boost/tr1/type_traits.hpp>
#include <boost/type_traits/extent.hpp>
#include <boost/type_traits/is_class.hpp>
#include <boost/type_traits/is_integral.hpp>
#include <boost/type_traits/is_array.hpp>
#include <boost/type_traits/is_floating_point.hpp>

#include <boost/mpl/if.hpp>

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
#include "util.hpp"

#define PULMOTOR_ADD_MARKERS 1
#define PULMOTOR_DEBUG_GATHER 1

#define	pulmotor_addressof(x) (&reinterpret_cast<unsigned char const&> (x))

namespace pulmotor {

	enum category {
		k_integral,
		k_pointer,
		k_floating_point,
		k_other
	};

inline char const* to_string (category cat) {
	switch (cat) {
		case k_integral: return "integral"; 
		case k_pointer: return "pointer";
		case k_floating_point: return "floating point";
		default:
		case k_other: return "other";
	}
}
		
using namespace boost::multi_index;
using namespace std::tr1;

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

inline void logf_indent (int diff, char const* fmt, ...) {
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

#if PULMOTOR_DEBUG_GATHER
#define gather_logf(...) logf(__VA_ARGS__)
#define gather_logf_indent(diff,...) logf_indent(diff,__VA_ARGS__)
#else
#define gather_logf(...)
#define gather_logf_indent(diff,...)
#endif

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

/*struct name_holder {
	char const* name_;
	name_holder (char const* name) : name_ (name) {}
};

inline name_holder n (char const* str) { return name_holder (str); }

template<class T>
struct nvp_t
{
	char const* name_;
	T const&	value_;

	nvp_t (name_holder const& n, T const& value) : name_ (n.name_), value_ (value) {}
};

template<class T>
inline nvp_t operator> (name_holder n, T const& value)
{
	return nvp_t (n, value);
}*/

//	ser	| name ("record")
//		| n("mama") > mama_
//		| n("pedro") > pedro_
//		;

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
struct clean : public remove_cv<T>
{
//	typedef typename boost::mpl::if_c<
//			is_pointer<T>::value,
//			clean<typename remove_pointer<T>::type >,
//			remove_cv<T>
//		>::type type;
};

struct blit_section_info
{
	int 	data_offset;
	int		fixup_offset, fixup_count;
	int		reserved;
};

template<class T>
struct get_category
{
	static const category value =
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
		const_cast<T&> (obj).serialize (ar, version);
	}
};

struct blit_section
{
	class class_info;

	// describes a class member
	struct member_info
	{
		member_info (class_info const* ci, size_t offset, size_t size, size_t count, category cat)
			:	offset_(offset), count_ (count), size_ (size), category_ (cat)
	   	{}
	   	
	   	bool operator== (member_info const& a) const {
	   		return a.ci_ == ci_ && a.offset_ == offset_ && a.count_ == count_ && a.size_ == size_ && a.category_ == category_;
	   	}

		class_info const*	ci_;
		size_t				offset_;
		size_t				count_;
		// TODO: probably make size + category smaller, like 32 bits total
		u32					size_;
		u32					category_;
	};

	struct ref
	{
		ref (size_t offset)
			:	offset_(offset)
		{}

		bool operator==(ref const& a) const { return offset_ == a.offset_; }
		size_t		offset_;
	};
	
	class class_info
	{
		typedef std::vector<member_info> member_container;

	public:
		typedef member_container::const_iterator member_iterator;

	private:
		std::type_info const*	ti_;
		size_t					size_, align_;
		member_container		members_;
		category				category_;
		bool					complete_;

	public:
		class_info (std::type_info const* ti, size_t size, size_t align)
		:	ti_(ti), size_(size), align_(align), complete_(false)
		{}

		size_t member_count () const { return members_.size (); }

		char const* get_name () const
		{	return ti_->name (); }

		bool is_complete () const
		{	return complete_; }
		void set_complete (bool complete)
		{	complete_ = complete; }

		void add_member (class_info const* ci, size_t offset, size_t size, size_t count, category cat)
		{
			// TODO: what to do with unions?
			// check for duplication (this will happen if a class has a pointer to the same type of object). eg. class S { S* ps; };
			member_info mi (ci, offset, size, count, cat);
			member_container::iterator it = std::find (members_.begin (), members_.end (), mi);
			if (it == members_.end ()) {
				gather_logf_indent (1, " + add member: offset: %lu, size: %lu, count: %lu, cat: %s (%s)\n", offset, size, count, to_string(cat), ci->get_name ());
				members_.push_back (mi);
			}
			gather_logf_indent (-1, 0);
		}

		void dump_members (std::string const& prefix) const
		{
			//logf ("> class '%s', %lu members\n", get_name (), members_.size ());
			for (size_t i=0; i<members_.size(); ++i)
			{
				member_info const& mi = members_[i];
				logf ("%smember +%03lu, size:%2d, count:%2lu, cat:%s\n", prefix.c_str(), mi.offset_, mi.size_, mi.count_, to_string((category)mi.category_));
			}
		}

		void change_endianess (void* ptr, size_t count)
		{
			while (count--)
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
							util::swap_endian<2> (p, mi.count_);
//							*(u16*)p = 0xAAAA;
							break;
						case 4:
							util::swap_endian<4> (p, mi.count_);
//							*(u32*)p = 0xBBBBBBBB;
							break;
	//					case 8:
	//						util::swap_endian<8> (p, mi.count_);
	//						break;
					}
				}

				ptr = (void*)((uintptr_t)ptr + size_);
			}
		}
		
		member_iterator member_begin () const { return members_.begin (); }
		member_iterator member_end () const { return members_.end (); }
		
		size_t get_size () const
		{	return size_; }

		size_t get_align () const
		{	return align_; }
	};

	// describes an instance (or an array) of a class in memory
	class object
	{
		class_info*		ci_;
		uintptr_t		data_;
		size_t			size_;
		size_t			count_;
		category		cat_;

	public:
		object (class_info*	ci, uintptr_t data, size_t size, size_t count, category cat)
			:	ci_(ci), data_(data), size_(size), count_ (count), cat_ (cat)
		{}
		
		category get_category () const { return cat_; }

		uintptr_t data () const { return data_; }

		// true if address belongs to this object
		bool belongs (uintptr_t ptr) const { return ptr >= data_ && ptr < data_ + full_size (); }
		size_t local_offset (uintptr_t ptr) const { assert(belongs(ptr)); return ptr - data_; }

		bool is_pointer () const { return cat_ == k_pointer; }
		bool is_class () const { return cat_ == k_other; }

		size_t count () const { return count_; }
		size_t full_size () const { return count_ * size_; }
		size_t size () const { return size_; }

		// can be raw memory with pointers and therefore do not have class_info
		class_info* get_class_info () const { return ci_; }
		char const* get_class_name () const { return ci_ ? ci_->get_name () : "<null>"; }
	};
	
	struct objectptr_less_addr {
		bool operator()(object const* a, object const* b) const {
			return a->data () < b->data ();
		}
		bool operator()(uintptr_t a, object const* b) const {
			return a < b->data ();
		}
		bool operator()(object const* a, uintptr_t b) const {
			return a->data () < b;
		}
	};

	struct select_first {
		template<class T1, class T2>
		T1 operator () (std::pair<T1, T2> const& a) const { return a.first; }
	};

	struct parent_info
	{
		// enum { compound, pointer, array };
		parent_info(class_info* ci, uintptr_t p, size_t fullsize, category cat, bool isarray)
		   : ci_(ci), object_ptr(p), fullsize_ (fullsize), cat_(cat), isarray_ (isarray)
	   	{}

		class_info* ci_;
		uintptr_t	object_ptr;
		size_t		fullsize_;
		category	cat_;
		bool		isarray_;

		bool is_array () const { return isarray_; }
		bool belongs (uintptr_t ptr) const { return ptr >= object_ptr && ptr < object_ptr + fullsize_; }
	};
	
	typedef std::vector<object*> object_container_t;
	typedef std::vector<parent_info> pi_container_t;
//	typedef std::map<void*, object*> object_map_t;
	
	struct ptr_tag {};
	typedef multi_index_container<
		object*,
		indexed_by<
			ordered_non_unique<tag<ptr_tag>, identity<object*>, objectptr_less_addr>
		>
	> object_mindex_t;
	
	typedef object_mindex_t::index<ptr_tag>::type	object_ptr_t;
	
	object_mindex_t		objects_;
	object_container_t	layout_parents_, roots_;
	pi_container_t		type_parents_;
	
	typedef std::map<std::type_info const*, class_info*> class_info_container_t;
	class_info_container_t class_infos_;
	
	blit_section ()
	{}

	~blit_section ()
	{
		for (class_info_container_t::iterator it = class_infos_.begin (), end = class_infos_.end (); it != end; ++it)
			delete it->second;
	}
	
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

/*		else
		{
			if (cat == k_pointer)
			{
				object* o = new object (ci, ptr, size, count, k_pointer, false);
				objects_.get<ptr_tag> ().insert (o);
				if (layout_parents_.empty ())
					roots_.push_back (o);
			}
		}
*/

	// pass a pointer to an object to check if that object is already registered. pointer passed can point to
	// the middle of an object.
	bool has_registered (void* p)
	{
		object_ptr_t::iterator it = objects_.get<ptr_tag> ().lower_bound ((uintptr_t)p, objectptr_less_addr());
		if (it != objects_.get<ptr_tag> ().end ())
			return (*it)->belongs ((uintptr_t)p);
		
		return false;
	}

	std::vector<std::string> parent_names_;
	std::string current_name_;
	void set_name (char const* name)
	{
		current_name_ = name;
	}

	bool requires_instance (uintptr_t ptr)
	{
		if (type_parents_.empty ())
			return true;

		if (layout_parents_.empty ())
			return true;

		if (layout_parents_.back ()->belongs (ptr))
			return false;

		return true;
	}

	void begin_area (class_info* ci, uintptr_t ptr, size_t object_size, size_t count, category cat, bool isarray = false)
	{
		parent_names_.push_back (current_name_);
		current_name_ = "";
		
		if (ci && !type_parents_.empty ())
		{
			// if instance parent is a composite, add this as a member
			parent_info const& pi = type_parents_.back ();
			if (pi.ci_ && !pi.ci_->is_complete () && pi.cat_ == k_other && !pi.is_array ())
			{
				uintptr_t pp = type_parents_.back ().object_ptr; 
				bool belongs = ptr >= pp && ptr < pp + pi.ci_->get_size ();
				// if belongs to parent...
				if (belongs)
				{
					uintptr_t offset = ptr - type_parents_.back ().object_ptr;
					type_parents_.back ().ci_->add_member (ci, offset, object_size, count, cat);
				}
			}
		}

		// if this is a root or an instance that does not belong to the parent, create 'object' for it.
		if (type_parents_.empty () || !type_parents_.back ().belongs (ptr))
		{
			gather_logf_indent (0, "= create object: %lu x %lu\n", object_size, count);
			object* o = new object (ci, ptr, object_size, count, cat);
			if (type_parents_.empty ())
				roots_.push_back (o);

			objects_.get<ptr_tag> ().insert (o);
		}

		type_parents_.push_back (parent_info(ci, ptr, object_size * count, cat, isarray));

		gather_logf_indent (0, "> BEGIN object %p, %lu x %lu (%s)\n", ptr, object_size, count, to_string (cat));
	}

	void end_area (class_info* ci, uintptr_t ptr)
	{
		parent_names_.pop_back ();
		type_parents_.pop_back ();

		if (ci && !ci->is_complete())
			ci->set_complete (true);
	
		gather_logf_indent (0, "> END object %p\n", ptr);
	}

	void dump_gathered ()
	{
		logf ("gathered- types:\n");
		for (class_info_container_t::iterator it = class_infos_.begin (), end = class_infos_.end (); it != end; ++it)
		{
			class_info const& ci = *it->second;
			logf ("  class (%p): '%s', %lu members, size: %d (%d)\n", it->second,
				shorten_name(ci.get_name ()).c_str(), ci.member_count (), ci.get_size (), ci.get_align());
			ci.dump_members ("    ");
		}

		logf ("pointer objects (roots: %lu):\n", roots_.size ());

		int i = 0;
		for (object_ptr_t::iterator it = objects_.get<ptr_tag>().begin (), end = objects_.get<ptr_tag>().end (); it != end; ++it)
		{
			object const* o = *it;
			object_container_t::iterator rootIt = std::find (roots_.begin (), roots_.end (), o);
			logf ("  [%03d] %s%p, size:%d x %d class: '%s', cat: %s\n",
				i, rootIt == roots_.end () ? "" : "-[ROOT]- ", o->data (), o->size (), o->count (),
			   	shorten_name (o->get_class_name ()).c_str (), to_string (o->get_category ()));
			++i;
		}
	}

	typedef std::vector<std::pair<uintptr_t, uintptr_t> > ptr_refs_t;
		
	void append (std::vector<unsigned char>& buffer, void const* data, size_t size)
	{
		buffer.insert (buffer.end (), (unsigned char const*)data, (unsigned char const*)data + size);
	}

	typedef std::map<object*, size_t> file_offsets_t;

	void write_out (std::vector<unsigned char>& output_buffer, bool change_endianess)
	{
//		std::sort (objects_.begin (), objects_.end (), objectptr_less_addr());

		size_t initialBufferSize = output_buffer.size ();

		// offsets to 'pointers' in the written stream
		ptr_refs_t pr;

		// object offsets in the buffer
		file_offsets_t fo;

		std::vector<unsigned char> data_buffer;
		for (object_container_t::iterator it = roots_.begin (), end = roots_.end (); it != end; ++it)
		{
			write_out (*it, data_buffer, pr, fo, change_endianess);
		}

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

		unsigned flags = change_endianess ? pulmotor::basic_header::flag_be : 0; 
		pulmotor::basic_header hdr (0x0001, pulmotor::basic_header::flag_plain | flags);

		blit_section_info bsi;
		bsi.reserved = 0xdddddddd;
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
			util::set_offsets (&*data_buffer.begin (), &*pr.begin (), pr.size (), change_endianess);
		logf ("data: %d bytes\n", data_buffer.size());

		if (!data_buffer.empty())
			append (output_buffer, &*data_buffer.begin (), data_buffer.size ());

#if PULMOTOR_ADD_MARKERS
		append (output_buffer, (unsigned char*)mark2, 16); 
#endif

		size_t fixups_offset = output_buffer.size ();
		if (!fixups.empty())
			append (output_buffer, &*fixups.begin (), fixups.size () * sizeof (uintptr_t));

		if (change_endianess)
			util::swap_endian<sizeof(uintptr_t)> (&output_buffer [fixups_offset], fixups.size());
			
		logf ("total: %d bytes\n", output_buffer.size () - initialBufferSize);
	}

	object* find_object_at (uintptr_t ptr)
	{
		object_ptr_t::iterator it = objects_.get<ptr_tag>().lower_bound (ptr, objectptr_less_addr ());
		
		if (it == objects_.get<ptr_tag>().end ())
			return 0;

		assert ((*it)->belongs (ptr));

		return *it;
	}

	uintptr_t fetch_pointer (std::vector<unsigned char>& buffer, size_t offset)
	{
		assert (offset <= buffer.size () - sizeof(size_t));
		u8 const* p = &buffer [offset];
		return *(uintptr_t const*)p;
	}

	uintptr_t write_out (object* o, std::vector<unsigned char>& buffer, ptr_refs_t& pr, file_offsets_t& fo, bool change_endianess)
	{
		file_offsets_t::iterator it = fo.find (o);
		if (it != fo.end ())
			return it->second;

		logf_indent (1, "obj: %p, full-size: %d, type: %s, cat: %s\n", o->data (), o->full_size(), o->get_class_name (), to_string(o->get_category()));

		// todo: align buffer

		// offset of this object in the output buffer (file)
		uintptr_t thisOffset = buffer.size ();
		fo.insert (std::make_pair (o, thisOffset));

		// insert the data of the object(s) into the buffer
		// we need to store the whole array as that has to be continuous
		buffer.insert (buffer.end (), (char*)o->data (), (char*)o->data () + o->full_size ());
//		if (change_endianess && o->get_category () != k_other)
//		{
//			logf_indent (0, "swap: 0x%08x, %lu x %lu\n", thisOffset, o->size (), o->count ());
//			util::swap_elements (&buffer [thisOffset], o->size (), o->count ());
//		}
		
		// pointer to the copied data of this object
		//logf_indent (0, "--> file at %p (buffer-addr: %p)\n", thisOffset, odat);

//		if (class_info* ci = o->get_class_info ())
//			ci->dump_members ();

		for (size_t ii=0; ii<o->count (); ++ii)
		{
			uintptr_t objectOffset = thisOffset + o->size () * ii; 
			switch (o->get_category ())
			{
				// if the object is a pointer, simply follow it
				case k_pointer:
				{
					uintptr_t ptr = fetch_pointer (buffer, objectOffset);
					logf_indent (0, "< POINTER: %p\n", ptr);

					object* o = find_object_at (ptr);
					if (!o)
					{
						logf_indent (0, "! object encompassing address %p is not registered\n", ptr);
						break;
					}

					uintptr_t written_offset = write_out (o, buffer, pr, fo, change_endianess);
					pr.push_back (std::make_pair (objectOffset, written_offset));
					break;
				}

				// object is a compound, traverse members and follow pointers is such exist
				case k_other:
				{
					class_info* ci = o->get_class_info ();
					assert (ci != NULL);
					for (class_info::member_iterator it = ci->member_begin (), end = ci->member_end (); it != end; ++it)
					{
						// fetch pointer to this object data again, as buffer may be changed by child's write_out
						logf_indent (0, "member-%02d: %s, size: %d\n", std::distance (ci->member_begin (), it), to_string ((category)it->category_), it->size_); 

						if (it->category_ != k_pointer)
							continue;

						// offset in the an already written buffer where the pointer lies
						uintptr_t ptr_offset = objectOffset + it->offset_;
						uintptr_t ptr = fetch_pointer (buffer, ptr_offset);

						logf_indent (0, "< POINTER: %p\n", ptr);

						object* o = find_object_at (ptr);
						if (!o)
						{
							logf_indent (0, "! object encompassing address %p is not registered\n", o);
							break;
						}

						int offset_in_referencee = o->local_offset (ptr);

						// WRITE IT!
						// offset of the object that 'this' reference is pointing to
						uintptr_t referencee_offset = write_out (o, buffer, pr, fo, change_endianess);
				
						pr.push_back (std::make_pair (ptr_offset, referencee_offset + offset_in_referencee));
					}

					if (change_endianess)
					{
						o->get_class_info ()->dump_members ("  <SWAP> ");
						ci->change_endianess (&*(buffer.begin () + objectOffset), 1);
					}

					break;
				}

				// otherwise do nothing
				default:
					break;

			}
		}

/*

		// walk through members of this class, patch the pointers (convert memory-address to file offset) and follow pointers
		for (class_info::member_iterator it = ci->member_begin (), end = ci->member_end (); it != end; ++it)
		{
			// fetch pointer to this object data again, as buffer may be changed by child's write_out
			odat = &*(buffer.begin () + thisOffset);
			logf_indent (0, "member-%02d: %s, size: %d\n", std::distance (ci->member_begin (), it), to_string ((category)it->category_), it->size_); 

			if (it->category_ != k_pointer)
				continue;

			// address of the pointer in the already copied data
			uintptr_t const* ptrAddr = (uintptr_t const*) (odat + it->offset_);
			uintptr_t ptr = *ptrAddr;

			logf_indent (0, "ptr: %p @%p\n", ptr, ptrAddr);

			// if pointer to object is null, do nothing
			if (ptr == 0) {
				logf_indent (0, "Pointer to object is null, skipping\n");
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
			if (!refObj.belongs (ptr))
			{
				logf ("ERROR: object %p (ptr to it at %p). Registered data pointer: %p, fullsize: %d\n", ptr, ptrAddr, refObj.data (), refObj.full_size ());
			}

			assert ((refObj.full_size () == 0 || (refObj.belongs (ptr))) && "Memory range declared by the referenced object does not encompass the reference");
			
			// reference may point into the middle of the object, remeber the offset
			int offsetInReferencee = refObj.local_offset (ptr);
			
			// pointer location in the stream (of already written object)
			uintptr_t ptrLocationOffset = it->offset_ + thisOffset;
			
			// offset of the object that 'this' reference is pointing to
			uintptr_t referenceeOffset = write_out (&refObj, buffer, pr, fo, change_endianess);
			
			pr.push_back (std::make_pair (ptrLocationOffset, referenceeOffset + offsetInReferencee));
		}
*/			
		logf_indent (-1, 0);
		
		return thisOffset;
	}
};

/*template<class T, class CreateDataT>
struct factory_creation
{
	factory_creation (void* ptr, CreateDataT const& data)
	:	id_ ((uintptr_t)ptr), data_ (data)
	{}

	uintptr_t	id_;	
	CreateDataT	data_;
};*/

template<class ArchiveT, class ObjectT>
inline void serialize (ArchiveT& ar, ObjectT& obj, unsigned long version)
{
	access::call_member (static_cast<ArchiveT&> (ar), obj, version);
}

// a structure
template<class ArchiveT, class ObjectT>
inline void blit_impl (ArchiveT& ar, ObjectT& obj, unsigned version, compound_tag)
{
	gather_logf_indent (0, "> by-value compound, type: %s\n",
		shorten_name(typeid(obj).name()).c_str());

	typedef typename clean<ObjectT>::type clean_t;

	assert (get_category<clean_t>::value == k_other);

	blit_section::class_info* ci = ar.template get_class_info<clean_t>();

	ar.begin_area (ci, (uintptr_t)pulmotor_addressof (obj), sizeof(obj), 1, get_category<clean_t>::value);
	gather_logf_indent (1, 0);
	serialize (ar, obj, version);
	gather_logf_indent (-1, 0);
	ar.end_area (ci, (uintptr_t)pulmotor_addressof (obj));
}

template<class ArchiveT, class ObjectT>
inline void blit_impl (ArchiveT& ar, ObjectT& obj, unsigned version, primitive_tag)
{
	gather_logf_indent (0, "> primitive %s\n", shorten_name(typeid(obj).name()).c_str());
	uintptr_t oa = (uintptr_t)pulmotor_addressof (obj); 
	typedef typename clean<ObjectT>::type clean_t;
	blit_section::class_info* ci = ar.template get_class_info<clean_t>();
	assert (get_category<clean_t>::value == k_integral || get_category<clean_t>::value == k_floating_point);
	ar.begin_area (ci, oa, sizeof(obj), 1, get_category<clean_t>::value);
	ar.end_area (ci, oa);
	gather_logf_indent (0, 0); 
}

// true, array of primitives
template<class ArchiveT, class ObjectT>
inline void blit_array_helper (ArchiveT& ar, ObjectT* obj, size_t array_size, unsigned version, boost::mpl::bool_<true>)
{
	// do nothing, or copy memory block
	gather_logf_indent (1, "> blit-array  (x%d), type: %s\n",
		array_size, shorten_name(typeid(obj).name()).c_str());

		ar.register_member ( (uintptr_t)pulmotor_addressof (obj), sizeof (*obj), array_size, get_category<ObjectT>::value);

	gather_logf_indent (-1, 0);
}

// false, array of classes or pointers
template<class ArchiveT, class ObjectT>
inline void blit_array_helper (ArchiveT& ar, ObjectT* obj, size_t array_size, unsigned version, boost::mpl::bool_<false>)
{
	gather_logf_indent (1, "> blit-array (x%d), type: %s\n",
		array_size, shorten_name(typeid(*obj).name()).c_str());
	for (size_t i=0; i<array_size; ++i)
	{
		blit (ar, obj [i]);
	}
	gather_logf_indent (-1, 0);
}

template<class ArchiveT, class ObjectT>
inline void blit_impl (ArchiveT& ar, ObjectT& obj, unsigned version, array_tag)
{
	gather_logf ("ARRAY ObjectT: %s\n", util::dm(typeid(ObjectT).name()).c_str());
	ObjectT* aa = (ObjectT*) pulmotor_addressof (obj);

	size_t const array_size = boost::extent<ObjectT>::value;
	typedef typename remove_all_extents<ObjectT>::type pointee_t;

	gather_logf_indent (0, "array (@ %p) x %lu, type: %s\n",
		aa, array_size, shorten_name(typeid(obj).name()).c_str());

	gather_logf_indent (0, "  >>>> pointee: %s\n", typeid(pointee_t).name ());
	
	blit_section::class_info* ci = ar.template get_class_info<typename clean<pointee_t>::type>();

	gather_logf_indent (1, 0);
	ar.begin_area (ci, (uintptr_t)aa, sizeof(obj[0]), array_size, get_category<typename clean<ObjectT>::type>::value, true);
	for (size_t i=0; i<array_size; ++i)
		blit (ar, obj[i]);
	ar.end_area (ci, (uintptr_t)aa);
	gather_logf_indent (-1, 0);
}

template<class ArchiveT, class ObjectT>
inline void blit_impl (ArchiveT& ar, ptr_address<ObjectT> objaddr, unsigned version, ptr_address_tag)
{
	ObjectT** oa = reinterpret_cast<ObjectT**> (objaddr.addr);

	gather_logf_indent (0, "pointer (%p x %lu @ %p), type: %s\n",
		*oa, objaddr.count, objaddr.addr, shorten_name(typeid(ObjectT).name()).c_str());

	typedef typename clean<ObjectT>::type pointee_t;
	gather_logf_indent (0, "  >>>> pointee: %s\n", typeid(pointee_t).name ());
	blit_section::class_info* ci = get_category<pointee_t>::value == k_other ? ar.template get_class_info<pointee_t>() : NULL;
	gather_logf_indent (1, 0);
	ar.begin_area (NULL, (uintptr_t)oa, sizeof (ObjectT*), 1, k_pointer);
	
	bool already_registered = ar.has_registered (*oa);

	if (already_registered)
		gather_logf_indent (0, "Object/ptr already registered\n");

	typedef typename boost::mpl::bool_<is_primitive<ObjectT>::value>::type is_prim_t;

	// process class only if pointer is valid and not registered yet
	if (!already_registered && *oa)
	{
		gather_logf_indent (1, 0);
		if (objaddr.count > 1)
			ar.begin_area (ci, (uintptr_t)*oa, sizeof (ObjectT), objaddr.count, get_category<pointee_t>::value, true);

		for (size_t i=0; i<objaddr.count; ++i)
			blit (ar, (*oa) [i]);
		//blit_array_helper (ar, *ppobj, objaddr.count, version, is_prim_t ());
		if (objaddr.count > 1)
			ar.end_area (ci, (uintptr_t)*oa);
		gather_logf_indent (-1, 0);
	}

	ar.end_area (NULL, (uintptr_t)oa);
	gather_logf_indent (-1, 0);
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
/*template<class ArchiveT, class ObjectT>
inline ArchiveT& blit_object (ArchiveT& ar, ObjectT& obj)
{
	unsigned const ver = version<typename clean<ObjectT>::type>::value;
	ar.begin_area (&obj, 1, ver, false);
	blit_redirect (ar, obj, ver);
	ar.end_area (&obj, ver, false);
	return ar;
}*/

template<class ArchiveT, class ObjectT>
inline ArchiveT& blit (ArchiveT& ar, ObjectT& obj)
{
	unsigned const ver = version<typename clean<ObjectT>::type>::value;
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

namespace util
{

inline pulmotor::blit_section_info* get_bsi (void* data, bool skipHeader = true)
{
	return (pulmotor::blit_section_info*) ((char*)data + (skipHeader ? 0 : sizeof (pulmotor::basic_header)));
}

template<class T>
inline void blit_to_container (T& a, std::vector<unsigned char>& odata, bool change_endianess)
{
	pulmotor::blit_section bs;
	bs | a;
	bs.write_out (odata, change_endianess);
}

template<class T>
inline T* fixup_pointers (pulmotor::blit_section_info* bsi)
{
	char* data = ((char*)bsi + bsi->data_offset);
	uintptr_t* fixups = (uintptr_t*)((char*)bsi + bsi->fixup_offset);

	util::fixup_pointers (data, fixups, bsi->fixup_count);

	T* pt = (T*)data;
	return pt;
}

}

} // pulmotor

#endif
