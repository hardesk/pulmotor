#ifndef STIR_SER_HPP_
#define STIR_SER_HPP_

#include "pulmotor_config.hpp"
#include "pulmotor_types.hpp"

#include <type_traits>
//#include <boost/type_traits/extent.hpp>
//#include <boost/type_traits/is_class.hpp>
//#include <boost/type_traits/is_integral.hpp>
//#include <boost/type_traits/is_array.hpp>
//#include <boost/type_traits/is_floating_point.hpp>

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

#define PULMOTOR_ADD_MARKERS 0
#define PULMOTOR_DEBUG_GATHER 0
#define PULMOTOR_DEBUG_WRITE 0

#define	pulmotor_addressof(x) (&reinterpret_cast<unsigned char const&> (x))

namespace pulmotor {

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



template<class T>
inline void blit_to_container (T& a, std::vector<unsigned char>& odata, target_traits const& tt);

void set_offsets (void* data, std::pair<uintptr_t,uintptr_t> const* fixups, size_t fixup_count, size_t ptrsize, bool change_endianess);
void fixup_pointers (void* data, uintptr_t const* fixups, size_t fixup_count);

template<class T> struct ptr_address;
template<class T> inline ptr_address<typename std::remove_cv<T>::type> ptr (T*& p, size_t cnt = 1);
//template<class T> inline ptr_address<T> ptr (T const* const& p, size_t cnt);
template<class ArchiveT, class ObjectT> inline ArchiveT& blit (ArchiveT& ar, ObjectT& obj);

template<class ArchiveT, class T>
inline typename pulmotor::enable_if<std::is_base_of<archive, ArchiveT>::value, ArchiveT>::type&
operator& (ArchiveT& ar, T const& obj);

template<class ArchiveT, class T>
inline typename pulmotor::enable_if<std::is_base_of<archive, ArchiveT>::value, ArchiveT>::type&
operator| (ArchiveT& ar, T const& obj);


template<class ObjectT>
inline blit_section& operator | (blit_section& ar, ObjectT const& obj);
template<class ObjectT>
inline blit_section& operator & (blit_section& ar, ObjectT const& obj);


namespace util
{
    void* fixup_pointers_impl (pulmotor::blit_section_info* bsi);

    template<class T>
    inline T* fixup_pointers (pulmotor::blit_section_info* bsi);

    template<class T>
    T* fixup_pointers (void* dataWithHeader);

    inline void fixup (pulmotor::blit_section_info* bsi);

    //	inline size_t write_file (path_char const* name, u8 const* ptr, size_t size);

    template<class T>
    size_t write_file (path_char const* name, T& root, target_traits const& tt, size_t sectionalign = 16);
}


struct blit_section;

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

inline void change_endianess (blit_section_info& bsi)
{
    util::swap_variable (bsi.data_offset);
    util::swap_variable (bsi.fixup_offset);
    util::swap_variable (bsi.fixup_count);
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

#if PULMOTOR_DEBUG_GATHER
#define gather_logf(...) logf(__VA_ARGS__)
#define gather_logf_ident(diff,...) logf_ident(diff,__VA_ARGS__)
#else
#define gather_logf(...)
#define gather_logf_ident(diff,...)
#endif

#if PULMOTOR_DEBUG_WRITE
#define write_logf(...) logf(__VA_ARGS__)
#define write_logf_ident(diff,...) logf_ident(diff,__VA_ARGS__)
#else
#define write_logf(...)
#define write_logf_ident(diff,...)
#endif

enum { pointer_size = sizeof(void*) * 8 };

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
inline nvp_t operator/ (name_holder n, T const& value)
{
    return nvp_t (n, value);
}*/

//	ser	| name ("record")
//		| n("mama") / mama_
//		| n("pedro") / pedro_
//		;

/*template<class Target, class Original>
struct exchange_t
{
    exchange_t (Target const& t, Original const& o)
    :	target (t), original (o)
    {}

    Target		value;
    Original	original;
};*/

template<class T>
inline ptr_address<typename std::remove_cv<T>::type> ptr (T*& p, size_t cnt)
{
    typedef typename std::remove_cv<T>::type clean_t;
    return ptr_address<clean_t> ((clean_t**)pulmotor_addressof(p), cnt);
}
/*template<class T>
inline ptr_address<typename std::tr1::remove_cv<T>::type> ptr (T const* const& p, size_t cnt = 1)
{
    typedef typename std::tr1::remove_cv<T>::type clean_t;
    return ptr_address<clean_t> ((clean_t* const*)pulmotor_addressof(p), cnt);
}*/

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
    public boost::type_traits::ice_or<std::is_integral<T>::value, std::is_floating_point<T>::value>
{};

//template<class T>
//struct is_exchange : public boost::mpl::bool_<false> {}
//template<class T, class O>
//struct is_exchange<exchange_t<T, O> > : public boost::mpl::bool_<true> {}


template<class T>
struct clean : public std::remove_cv<T>
{
//	typedef typename boost::mpl::if_c<
//			is_pointer<T>::value,
//			clean<typename remove_pointer<T>::type >,
//			remove_cv<T>
//		>::type type;
};

template<class T>
struct type_category
{
    static const category value =
        std::is_integral<T>::value		? k_integral
        : std::is_floating_point<T>::value	? k_floating_point
        : std::is_pointer<T>::value			? k_pointer
        : k_other
        ;
};

struct blit_section
{
    class class_info;

    // describes a class member
    struct member_info
    {
        member_info (class_info const* ci, size_t offset, size_t count)
            :	ci_ (ci), offset_(offset), count_ (count)
            {}

            bool operator== (member_info const& a) const {
                return a.ci_ == ci_ && a.offset_ == offset_ && a.count_ == count_;
            }

        category get_category () const { return ci_->get_category(); }
        bool is_array () const { return count_ > 1; }
        size_t get_size () const { return ci_->get_size(); }

        class_info const*	ci_;
        size_t				offset_;
        size_t				count_;
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

#if PULMOTOR_DEBUG_GATHER || PULMOTOR_DEBUG_WRITE
        char const*	class_name_;
#endif

    public:
        class_info (std::type_info const* ti, category cat, size_t size, size_t align)
        :	ti_(ti), size_(size), align_(align), category_(cat), complete_(false)
        {
#if PULMOTOR_DEBUG_GATHER || PULMOTOR_DEBUG_WRITE
            class_name_ = get_name ();
#endif
        }

        category get_category () const { return category_; }
        size_t member_count () const { return members_.size (); }
        bool no_members () const { return members_.empty (); }

        bool is_pointer () const { return category_ == k_pointer; }

        char const* get_name () const
        {	return ti_->name (); }

        bool is_complete () const {	return complete_; }
        void set_complete (bool complete) {	complete_ = complete; }

        void add_member (class_info const* ci, size_t offset, size_t size, size_t count, category cat)
        {
            // TODO: what to do with unions?
            // check for duplication (this will happen if a class has a pointer to the same type of object). eg. class S { S* ps; };
            member_info mi (ci, offset, count);
            member_container::iterator it = std::find (members_.begin (), members_.end (), mi);
            if (it == members_.end ()) {
//				// check for overlap:
//				for (member_container::iterator mit = members_.begin (), end = members_.end (); mit != end; ++mit) {
//					member_info const& mmi = *mit;
//
//					size_t mmie = mmi.offset_ + mmi.size_;
//					size_t mie = mi.offset_ + mi.size_;
//
//					if ((mi.offset_ >= mmi->offset_ && mi.offset_ < mmie) ||
//						(mi.offset
//				}

                gather_logf_ident (1, " + add member: cat: %s, offset: %lu, size: %lu, count: %lu, type:'%s'\n", to_string(cat), offset, size, count, ci ? ci->get_name () : "<null>");
                members_.push_back (mi);
            }
            gather_logf_ident (-1, 0);
        }

        void dump_members (std::string const& prefix) const
        {
            //write_logf ("> class '%s', %lu members\n", get_name (), members_.size ());
            for (size_t i=0; i<members_.size(); ++i)
            {
                member_info const& mi = members_[i];
                PULMOTOR_UNUSED(mi);
                write_logf ("%smember +%03lu, cat:%s, size:%2d, count:%2lu\n", prefix.c_str(), mi.offset_, to_string(mi.get_category()), mi.get_size(), mi.count_);
            }
        }

        member_iterator member_begin () const { return members_.begin (); }
        member_iterator member_end () const { return members_.end (); }

        size_t get_size () const
        {	return size_; }

        size_t get_alignment () const
        {	return align_; }
    };

    // describes an instance (or an array) of a class in memory
    class object
    {
        class_info*		ci_;
        uintptr_t		data_;
        size_t			size_; // = class::size
        size_t			count_; // for dynamically allocated 'arrays'

    public:
        object (class_info*	ci, uintptr_t data, size_t size, size_t count)
            :	ci_(ci), data_(data), size_(size), count_ (count)
        {}

        category get_category () const { return ci_->get_category (); }

        uintptr_t data () const { return data_; }

        // true if address belongs to this object
        bool belongs (uintptr_t ptr) const { return ptr >= data_ && ptr < data_ + full_size (); }
        size_t local_offset (uintptr_t ptr) const { assert(belongs(ptr)); return ptr - data_; }

        bool is_pointer () const { return ci_->get_category() == k_pointer; }
        bool is_class () const { return ci_->get_category() == k_other; }
        bool is_array () const { return count_ > 1; }

        size_t count () const { return count_; }
        size_t full_size () const { return count_ * size_; }
        size_t size () const { return size_; }

        // TODO: add per-object alignment
        size_t alignment (size_t defaultAlign) const
            {
            if (ci_ == 0)
                return object_alignment () == 0 ? defaultAlign : object_alignment ();
            else
                return object_alignment () == 0 ? ci_->get_alignment () : object_alignment ();
            }

        size_t object_alignment () const { return 0; }

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
        parent_info(class_info* ci, uintptr_t p, size_t fullsize, bool compounded, bool array)
            : ci_(ci), object_ptr(p), fullsize_ (fullsize), compounded_ (compounded), array_(array)
            {
#if PULMOTOR_DEBUG_GATHER || PULMOTOR_DEBUG_WRITE
            class_name_ = ci->get_name ();
#endif
        }

        class_info* ci_;
        uintptr_t	object_ptr;
        size_t		fullsize_;
        bool		compounded_; // it's a compound value of another object
        bool		array_;

#if PULMOTOR_DEBUG_GATHER || PULMOTOR_DEBUG_WRITE
        char const*	class_name_;
#endif

        bool is_array () const { return array_; }
        bool belongs (uintptr_t ptr) const { return ptr >= object_ptr && ptr < object_ptr + fullsize_; }
    };

    typedef std::vector<object*> object_container_t;
    typedef std::vector<parent_info> pi_container_t;
    // for holding offsets in the file for the objects registered
    typedef std::map<object*, size_t> file_offsets_t;
    // for tracking references. holds offsets in the files marking places where all the pointers/references are
    typedef std::vector<std::pair<uintptr_t, uintptr_t> > ptr_refs_t;

//	typedef std::map<void*, object*> object_map_t;

    struct ptr_tag {};
    typedef multi_index_container<
        object*,
        indexed_by<
            ordered_non_unique<tag<ptr_tag>, identity<object*>, objectptr_less_addr>
        >
    > object_mindex_t;

    typedef object_mindex_t::index<ptr_tag>::type	object_ptr_t;

    // objects registered by the recursive blit
    object_mindex_t		objects_;
    // root objects of the traversed hierarchy. roots are also present in objects_
    object_container_t	roots_;
    // when traversing, this holds parent object-types as a stack
    pi_container_t		type_parents_;

    // all types encountered by the blit
    typedef std::map<std::type_info const*, class_info*> class_info_container_t;
    class_info_container_t class_infos_;

    // name of a blit-scope. used for debugging
    std::vector<std::string> parent_names_;
    std::string current_name_;

    // METHODS
    blit_section ()
    :	target_pointer_size_ (sizeof(uintptr_t))
    {}

    ~blit_section ()
    {
        for (class_info_container_t::iterator it = class_infos_.begin (), end = class_infos_.end (); it != end; ++it)
            delete it->second;

        typedef object_mindex_t::index<ptr_tag>::type cont_t;
        for (cont_t::iterator it = objects_.get<ptr_tag>().begin (), end = objects_.get<ptr_tag>().end (); it != end; ++it)
            delete *it;
    }

    template<class T>
    class_info* get_class_info ()
    {
        class_info_container_t::iterator it = class_infos_.find (&typeid(T));
        if (it == class_infos_.end ()) {
            typedef typename clean<T>::type clean_t;
            it = class_infos_.insert (
                std::make_pair (
                    &typeid(T),
                        new class_info(&typeid(T), type_category<clean_t>::value, sizeof(T), 4))
            ).first;
        }
        return it->second;
    }

    // pass a pointer to an object to check if that object is already registered. pointer passed can point to
    // the middle of an object.
    bool has_registered (void const* p)
    {
        object_ptr_t::iterator it = objects_.get<ptr_tag> ().lower_bound ((uintptr_t)p, objectptr_less_addr());
        if (it != objects_.get<ptr_tag> ().end ())
            return (*it)->belongs ((uintptr_t)p);

        return false;
    }

    void set_name (char const* name)
    {
        current_name_ = name;
    }

    void begin_area (class_info* area_ci, uintptr_t ptr, size_t object_size, size_t count, category cat, bool array)
    {
        parent_names_.push_back (current_name_);
        current_name_ = "";

        if (!type_parents_.empty ())
        {
            // if instance parent is a composite, add this as a member
            parent_info const& pi = type_parents_.back ();

            bool belongs = pi.belongs (ptr);

            if (!pi.ci_->is_complete () && belongs)
            {
                if (pi.array_)
                {
//					if (pi.compounded_)
//					{
//						uintptr_t offset = ptr - pi.object_ptr;
//						pi.ci_->add_member (area_ci, offset, object_size, count, cat);
//					}
                } else {
                    if (pi.ci_->get_category() == k_pointer)
                    {
//						pi.ci_->add_member (area_ci, 0, object_size, count, cat);
                    } else if (pi.ci_->get_category() == k_other)
                    {
                        // in case of allocated array, its class_info is the same as of contained class'
                        if (pi.ci_ != area_ci)
                        {
                            uintptr_t offset = ptr - pi.object_ptr;
                            pi.ci_->add_member (area_ci, offset, object_size, count, cat);
                        }
                    }
                }
            }
        }

        // if this is a root or an instance that does not belong to the parent, create 'object' for it.
        if (type_parents_.empty () || !type_parents_.back ().belongs (ptr))
        {
            gather_logf_ident (0, "= create object: %lu x %lu, cat:%s\n", object_size, count, to_string (area_ci->get_category()));
            object* o = new object (area_ci, ptr, object_size, count);
            if (type_parents_.empty ())
                roots_.push_back (o);

            objects_.get<ptr_tag> ().insert (o);
        }

        bool compounded = type_parents_.empty() ? true : type_parents_.back ().belongs (ptr);
        type_parents_.push_back (parent_info(area_ci, ptr, object_size * count, compounded, array));
        if (!type_parents_.empty ()) {
            for (size_t i=0; i<type_parents_.size (); ++i)
                gather_logf ("%s(%s%s):", type_parents_[i].ci_->get_name(), to_string(type_parents_[i].ci_->get_category()),
                            type_parents_[i].array_ ? "[]" : "");
            gather_logf ("\n");
        }

        gather_logf_ident (0, "> BEGIN object %p, %lu x %lu (%s)\n", ptr, object_size, count, to_string (cat));
    }

    void end_area (class_info* ci, uintptr_t ptr)
    {
        parent_names_.pop_back ();
        type_parents_.pop_back ();

        // an array has member of one type (but may have many). stop type recording after one is finished
        if (!type_parents_.empty () && type_parents_.back ().is_array ())
            type_parents_.back ().ci_->set_complete(true);

        if (ci && !ci->is_complete())
            ci->set_complete (true);

        gather_logf_ident (0, "> END object %p\n", ptr);
    }

    void add_pointed_area (class_info* area_ci, uintptr_t ptr, size_t object_size, size_t count, category cat)
    {
        assert (!type_parents_.empty ());
        assert (!type_parents_.back ().ci_->is_pointer ());

        parent_info const& pi = type_parents_.back ();

        if (!pi.ci_->is_complete ())
        {
            pi.ci_->add_member (area_ci, 0, object_size, count, cat);
        }
    }

/*	void dump_gathered ()
    {
        write_logf ("gathered- types:\n");
        for (class_info_container_t::iterator it = class_infos_.begin (), end = class_infos_.end (); it != end; ++it)
        {
            class_info const& ci = *it->second;
            write_logf ("  class (%p): '%s', cat: %s, %lu members, size: %d (%d)\n", it->second,
                shorten_name(ci.get_name ()).c_str(), to_string (ci.get_category()), ci.member_count (), ci.get_size (), ci.get_alignment());
            ci.dump_members ("    ");
        }

        write_logf ("pointer objects (roots: %lu):\n", roots_.size ());

        int i = 0;
        for (object_ptr_t::iterator it = objects_.get<ptr_tag>().begin (), end = objects_.get<ptr_tag>().end (); it != end; ++it)
        {
            object const* o = *it;
            object_container_t::iterator rootIt = std::find (roots_.begin (), roots_.end (), o);
            write_logf ("  [%03d] %s%p, cat: %s, size:%d x %d class: '%s'\n",
                i, rootIt == roots_.end () ? "" : "-[ROOT]- ", o->data (), to_string (o->get_category ()), o->size (), o->count (),
                    shorten_name (o->get_class_name ()).c_str ());
            ++i;
        }
    }*/

    void append (std::vector<unsigned char>& buffer, void const* data, size_t size)
    {
        buffer.insert (buffer.end (), (unsigned char const*)data, (unsigned char const*)data + size);
    }

    static size_t align_on (size_t addr, size_t align)
    {
        return (addr - 1 | align - 1) + 1;
    }

    static bool is_aligned (size_t addr, size_t align)
    {
        return (addr & align - 1) == 0;
    }

    size_t align_buffer (std::vector<unsigned char>& output_buffer, size_t offset, size_t mismatch)
    {
        size_t actual_data = output_buffer.size () - mismatch;
        int tofill = (int)offset - (int)actual_data;
        if (tofill < 0)
        {
            // TODO: fatal error, pointers point wrong
            return 0;
        }

        char buf [64];
        memset (buf, 0, sizeof(buf));
        for(int left = tofill; left > 0; )
        {
            size_t write = (std::min) (sizeof buf, (size_t)left);
            append (output_buffer, buf, write);
            left -= write;
        }

        return tofill;
    }

    struct write_state
    {
        typedef std::vector<u8> buffer_t;

        buffer_t&				buffer;
        ptr_refs_t&				ptr_refs;
        file_offsets_t&			file_offsets; // offsets of the objects in file
        size_t					default_align;
        size_t					waste;
        target_traits const&	traits;

        write_state (buffer_t& obuf, ptr_refs_t& pr, file_offsets_t& fo, target_traits const& tt, size_t default_align_)
        :	buffer (obuf), ptr_refs (pr), file_offsets (fo), default_align (default_align_), waste (0), traits (tt)
        {
        }

        bool change_endianess () const {
            return
                (pulmotor::is_le () && traits.big_endian) ||
                (pulmotor::is_be () && !traits.big_endian);
        }
    };

    void write_out (std::vector<unsigned char>& output_buffer, target_traits const& targetTraits, size_t section_align = 4)
    {
//		std::sort (objects_.begin (), objects_.end (), objectptr_less_addr());

        size_t waste = 0;
        size_t initialBufferSize = output_buffer.size ();
        (void)initialBufferSize;

        // offsets to 'pointers' in the written stream
        ptr_refs_t pr;

        // object offsets in the buffer
        file_offsets_t fo;

        std::vector<unsigned char> data_buffer;
        write_state ws (data_buffer, pr, fo, targetTraits, 4);

        for (object_container_t::iterator it = roots_.begin (), end = roots_.end (); it != end; ++it)
        {
            write_out_object (*it, ws);
        }

        std::vector<uintptr_t> fixups;
        std::transform (pr.begin (), pr.end (), std::back_inserter(fixups), select_first ());

        std::sort (fixups.begin (), fixups.end ());

        if (!pr.empty())
            util::set_offsets (&*data_buffer.begin (), &*pr.begin (), pr.size (), targetTraits.ptr_size, ws.change_endianess ());


        for (size_t i=0; i<fixups.size (); ++i) {
            write_logf ("fixup [%2d] at: 0x%08x\n", i, fixups[i]);
        }

#if PULMOTOR_ADD_MARKERS
        size_t const marker_size = 8;
        char mark1 [marker_size+1] = "--data--";
        char mark2 [marker_size+1] = "-fixups-";
#endif
        size_t f_data_offset, f_fixup_offset, f_begin;

        // declare a new scope as we might change endianess and objects will be unusable
        {
            unsigned flags = targetTraits.big_endian ? pulmotor::basic_header::flag_be : 0;
            pulmotor::basic_header hdr (0x0001, pulmotor::basic_header::flag_plain | flags);

            blit_section_info bsi;

            f_data_offset = align_on (
#if PULMOTOR_ADD_MARKERS
                marker_size +
#endif
                sizeof bsi
                , section_align);

            f_fixup_offset = align_on (
#if PULMOTOR_ADD_MARKERS
                marker_size +
#endif
                f_data_offset + data_buffer.size ()
                , section_align);

            bsi.reserved = 0xdddddddd;
            bsi.data_offset = (pulmotor::u32)f_data_offset;
            bsi.fixup_offset= (pulmotor::u32)f_fixup_offset;
            bsi.fixup_count	= (pulmotor::u32)fixups.size ();

            // writing the header in BE, always
            if (pulmotor::is_le ())
                hdr.change_endianess ();

            if (ws.change_endianess ())
                change_endianess (bsi);

            append (output_buffer, &hdr, sizeof hdr);

            f_begin = output_buffer.size ();
            append (output_buffer, &bsi, sizeof bsi);
        }

#if PULMOTOR_ADD_MARKERS
        append (output_buffer, (unsigned char*)mark1, marker_size);
#endif

        if (!data_buffer.empty())
        {
            waste += align_buffer (output_buffer, f_data_offset, f_begin);
            append (output_buffer, &*data_buffer.begin (), data_buffer.size ());
        }

        if (!fixups.empty())
        {
#if PULMOTOR_ADD_MARKERS
            append (output_buffer, (unsigned char*)mark2, marker_size);
#endif

            waste += align_buffer (output_buffer, f_fixup_offset, f_begin);
            append (output_buffer, &*fixups.begin (), fixups.size () * sizeof (uintptr_t));
        }

        if (ws.change_endianess ())
            util::swap_endian<sizeof(uintptr_t)> (&output_buffer [f_fixup_offset + f_begin], fixups.size());

        write_logf ("total: %d bytes, waste: %d bytes\n", output_buffer.size () - initialBufferSize, waste);
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

    void process_pointer (object& o, size_t objectOffset, write_state& ws)
    {
    }


    void process_members (class_info const* ci, size_t objectOffset, write_state& ws)
    {
        for (class_info::member_iterator it = ci->member_begin (), end = ci->member_end (); it != end; ++it)
        {
            // fetch pointer to this object data again, as buffer may be changed by child's write_out
            write_logf_ident (0, "member(m)-%02d: %s, %s, size: %d\n",
                std::distance (ci->member_begin (), it), it->ci_->get_name (),
                to_string (it->get_category()), it->get_size());

            member_info const& mi = *it;
            for (size_t i=0; i<mi.count_; ++i)
            {
                // offset in the already written buffer where the pointer lies
                uintptr_t memberOffset = objectOffset + mi.offset_ + mi.get_size() * i;

                switch (mi.get_category())
                {
                    case k_other:
                    {
                        write_logf_ident (1, 0);
                        process_members (mi.ci_, memberOffset, ws);
                        write_logf_ident (-1, 0);
                        break;
                    }

                    case k_pointer:
                    {
                        uintptr_t ptr = fetch_pointer (ws.buffer, memberOffset);
                        write_logf_ident (0, "< POINTER: %p\n", ptr);

                        if (!ptr)
                        {
                            write_logf_ident (0, "< ZERO, not following\n");
                            break;
                        }

                        object* o = find_object_at (ptr);
                        if (!o)
                        {
                            write_logf_ident (0, "! object at address %p is not registered\n", o);
                            break;
                        }

                        size_t offset_in_referencee = o->local_offset (ptr);

                        write_logf_ident (1, 0);

                        // WRITE IT!
                        // offset of the object that 'this' reference is pointing to
                        uintptr_t referencee_offset = write_out_object (o, ws);
                        ws.ptr_refs.push_back (std::make_pair (memberOffset, referencee_offset + offset_in_referencee));
                        write_logf_ident (-1, 0);
                        break;
                    }

                    default:
                        if (ws.change_endianess ())
                        {
                        //	o->get_class_info ()->dump_members ("  <SWAP> ");
                            size_t mofs = objectOffset + mi.offset_;
                            util::swap_elements (&ws.buffer [mofs], mi.get_size(), mi.count_);
                        }
                        break;
                }
            }
        }
    }

//	void process_array (object& o, size_t objectOffset, write_state& ws)
//	{
//		assert (o.get_category() == k_other);
//
//		for (int oi=0; oi<o.count (); ++oi)
//		{
//			// fetch pointer to this object data again, as buffer may be changed by child's write_out
//			write_logf_ident (0, "  element-%02d: size %d, \n", oi, o.size());
//
//			// offset in the already written buffer where the pointer lies
//			uintptr_t elementOffset = objectOffset + o.size () * i;
//
//			switch (o.get_category ())
//			{
//				case k_other:
//					{
//						write_logf_ident (1, 0);
//						process_members (o.ci_, elementOffset, ws);
//						write_logf_ident (-1, 0);
//						break;
//					}
//
//				case k_pointer:
//				{
//					uintptr_t ptr = fetch_pointer (ws.buffer, memberOffset);
//					write_logf_ident (0, "< POINTER: %p\n", ptr);
//
//					if (!ptr)
//					{
//						write_logf_ident (0, "< ZERO, not following\n");
//						break;
//					}
//
//					object* o = find_object_at (ptr);
//					if (!o)
//					{
//						write_logf_ident (0, "! object at address %p is not registered\n", o);
//						break;
//					}
//
//					int offset_in_referencee = o->local_offset (ptr);
//
//					write_logf_ident (1, 0);
//
//					// WRITE IT!
//					// offset of the object that 'this' reference is pointing to
//					uintptr_t referencee_offset = write_out_object (o, ws);
//					ws.ptr_refs.push_back (std::make_pair (memberOffset, referencee_offset + offset_in_referencee));
//					write_logf_ident (-1, 0);
//					break;
//				}
//
//					default:
//						if (ws.change_endianess ())
//						{
//							//	o->get_class_info ()->dump_members ("  <SWAP> ");
//							size_t mofs = objectOffset + mi.offset_;
//							util::swap_elements (&ws.buffer [mofs], mi.size_, mi.count_);
//						}
//						break;
//				}
//			}
//		}
//	}

    static const size_t runtime_ptr_size = sizeof (uintptr_t) * 8;
    size_t target_pointer_size_;

    void out_pointer (uintptr_t dataPtr, write_state& ws)
    {
        if (ws.traits.ptr_size == pointer_size)
        {
            ws.buffer.insert (ws.buffer.end (), (char*)dataPtr, (char*)dataPtr + pointer_size);
        }
        else
        {
            // TODO: pointer remap to other arch
        }
    }

    void out_direct (uintptr_t dataPtr, size_t size, write_state& ws)
    {
        ws.buffer.insert (ws.buffer.end (), (char*)dataPtr, (char*)dataPtr + size);
    }

//	void write_object (object* o, write_state& ws)
//	{
//		class_info* ci = o->get_class_info ();
//
//		for (int oi=0; oi<o->count (); ++oi)
//		{
//			switch (o->get_category())
//			{
//				case k_pointer:
//					out_pointer (o->data(), ws);
//					break;
//
//				case k_floating_point:
//				case k_integral:
//					out_direct (o->data (), o->size(), ws);
//					break;
//
//					k_other
//
//			}
//		}
//
//
//		if (runtime_ptr_size != target_pointer_size)
//		{
//			//			class_info* ci = o->get_class_info ();
//			//			assert (ci != 0);
//			//			size_t obj_size = 0;
//			//			size_t size_modified = 0;
//			//			for (class_info::member_iterator it = ci->member_begin (), end = ci->member_end (); it != end; ++it)
//			//			{
//			//				member_info const& mi = *it;
//			//				switch (mi.get_category ())
//			//				{
//			//					case k_pointer:
//			//					{
//			//						break;
//			//					}
//			//
//			//					case k_other:
//			//					{
//			//						break;
//			//					}
//			//
//			//					default:
//			//					{
//			//						break;
//			//					}
//			//				}
//			//			}
//		}
//		else
//		{
//			ws.buffer.insert (ws.buffer.end (),
//							  (char*)o->data () + objIndex * o->size(),
//							  (char*)o->data () + (objIndex + 1) * o->size());
//		}
//	}

    void write_value (object*o, write_state& ws)
    {
        out_direct (o->data (), o->full_size(), ws);
    }

    void process_object (size_t objectOffsetInOutput, object* o, write_state& ws)
    {
        switch (o->get_category ())
        {
            // if the object is a pointer, simply follow it, unless it is zero, then do nothing
            case k_pointer:
            {
                for (size_t ii=0; ii<o->count (); ++ii)
                {
                    size_t objectOffset = objectOffsetInOutput + o->size() * ii;
                    // TODO: 64/32bit
                    uintptr_t ptr = fetch_pointer (ws.buffer, objectOffset);
                    write_logf_ident (0, "< POINTER: %p\n", ptr);

                    if (!ptr)
                    {
                        write_logf_ident (0, "< ZERO, not following\n");
                        break;
                    }

                    object* o = find_object_at (ptr);
                    if (!o)
                    {
                        write_logf_ident (0, "! object encompassing address %p is not registered\n", ptr);
                        break;
                    }

                    write_logf_ident (1, 0);
                    uintptr_t written_offset = write_out_object (o, ws);
                    write_logf_ident (0, "< REF in file: *%p = %p -> %p\n", objectOffset, ptr, written_offset);
                    ws.ptr_refs.push_back (std::make_pair (objectOffset, written_offset));
                    write_logf_ident (-1, 0);
                }
                break;
            }

                // object is a compound, traverse members and follow pointers if such exist
            case k_other:
            {
                write_logf_ident (1, 0);
                for (size_t ii=0; ii<o->count (); ++ii)
                {
                    size_t objectOffset = objectOffsetInOutput + o->size() * ii;
                    process_members (o->get_class_info (), objectOffset, ws);
                }
                write_logf_ident (-1, 0);
                break;
            }

                // otherwise just fix endianess if needed
            default:
            {
                if (ws.change_endianess ())
                {
//					for (size_t ii=0; ii<o->count (); ++ii)
//					{
//						void* ptr = &ws.buffer [objectOffset + ii * o->size ()];
                        void * ptr = &ws.buffer [objectOffsetInOutput];
                        util::swap_elements (ptr, o->size (), o->count ());
//					}
                }
                break;
            }
        }
    }

    uintptr_t write_out_object (object* o, write_state& ws)
    {
        file_offsets_t::iterator it = ws.file_offsets.find (o);
        if (it != ws.file_offsets.end ())
            return it->second;

        write_logf_ident (1, "obj: %p, full-size: %d, type: %s, cat: %s\n", o->data (), o->full_size(), o->get_class_name (), to_string(o->get_category()));

        // offset of this object in the output buffer (file)
        uintptr_t unaligned_this_offset = ws.buffer.size ();

        // align the buffer first on class alignment. use default-alignment if the class info is null.
        size_t object_align = o->alignment (ws.default_align);
        size_t this_offset = align_on (unaligned_this_offset, object_align);

        // remember offset of the object in the buffer
        ws.file_offsets.insert (std::make_pair (o, this_offset));

        // we passs 0 as mismatch offset as ws.buffer *must* be empty on first write_out
        ws.waste += align_buffer (ws.buffer, this_offset, 0);

        assert (is_aligned (ws.buffer.size (), object_align));

        // insert the data of the object(s) into the buffer.
        // we need to store the whole object as that can be an array
        // and that has to be continuous.

        // TODO: do we need to align per object? (say an array of structs)
        write_value (o, ws);

        // now process the inner data of the object (or an array of objects)
        // offset in the file of the current object we're processing
        uintptr_t objectOffsetInOutput = this_offset;// + o->size () * ii;
        process_object (objectOffsetInOutput, o, ws);

        write_logf_ident (-1, 0);

        return this_offset;
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

template<class ObjectT>
inline void serialize (blit_section& ar, ObjectT& obj, unsigned long version)
{
    obj.serialize (ar, version);
}

// a structure
template<class ObjectT>
inline void blit_impl (blit_section& ar, ObjectT& obj, unsigned version, compound_tag)
{
    gather_logf_ident (0, "> by-value compound, type: %s\n",
        shorten_name(typeid(obj).name()).c_str());

    typedef typename clean<ObjectT>::type clean_t;

    assert (type_category<clean_t>::value == k_other);

    blit_section::class_info* ci = ar.template get_class_info<clean_t>();

    ar.begin_area (ci, (uintptr_t)pulmotor_addressof (obj), sizeof(obj), 1, type_category<clean_t>::value, false);
    gather_logf_ident (1, 0);
    serialize (ar, obj, version);
    gather_logf_ident (-1, 0);
    ar.end_area (ci, (uintptr_t)pulmotor_addressof (obj));
}

template<class ObjectT>
inline void blit_impl (blit_section& ar, ObjectT& obj, unsigned version, primitive_tag)
{
    gather_logf_ident (0, "> primitive %s\n", shorten_name(typeid(obj).name()).c_str());
    uintptr_t oa = (uintptr_t)pulmotor_addressof (obj);
    typedef typename clean<ObjectT>::type clean_t;
    blit_section::class_info* ci = ar.template get_class_info<clean_t>();
    assert (type_category<clean_t>::value == k_integral || type_category<clean_t>::value == k_floating_point);
    ar.begin_area (ci, oa, sizeof(obj), 1, type_category<clean_t>::value, false);
    ar.end_area (ci, oa);
    gather_logf_ident (0, 0);
}

// true, array of primitives
/*template<class ObjectT>
inline void blit_array_helper (blit_section& ar, ObjectT* obj, size_t array_size, unsigned version, boost::mpl::bool_<true>)
{
    // do nothing, or copy memory block
    gather_logf_ident (1, "> blit-array  (x%d), type: %s\n",
        array_size, shorten_name(typeid(obj).name()).c_str());

        ar.register_member ( (uintptr_t)pulmotor_addressof (obj), sizeof (*obj), array_size, type_category<ObjectT>::value);

    gather_logf_ident (-1, 0);
}*/

// false, array of classes or pointers
template<class ObjectT>
inline void blit_array_helper (blit_section& ar, ObjectT* obj, size_t array_size, unsigned version, boost::mpl::bool_<false>)
{
    gather_logf_ident (1, "> blit-array (x%d), type: %s\n",
        array_size, shorten_name(typeid(*obj).name()).c_str());
    for (size_t i=0; i<array_size; ++i)
    {
        blit (ar, obj [i]);
    }
    gather_logf_ident (-1, 0);
}

template<class ObjectT>
inline void blit_impl (blit_section& ar, ObjectT& obj, unsigned version, array_tag)
{
    //gather_logf ("ARRAY ObjectT: %s\n", util::dm(typeid(ObjectT).name()).c_str());
    ObjectT* aa = (ObjectT*) pulmotor_addressof (obj);

    size_t const array_size = std::extent<ObjectT>::value;
    typedef typename std::remove_all_extents<ObjectT>::type array_member_t;

    gather_logf_ident (0, "array (@ %p) x %lu, type: %s\n",
        aa, array_size, shorten_name(typeid(obj).name()).c_str());

    gather_logf_ident (0, "  >>>> member: %s\n", typeid(array_member_t).name ());

    blit_section::class_info* ci = ar.template get_class_info<typename clean<array_member_t>::type>();

    gather_logf_ident (1, 0);
    category cat = type_category<typename clean<array_member_t>::type>::value;
    ar.begin_area (ci, (uintptr_t)aa, sizeof(obj[0]), array_size, cat, true);
    for (size_t i=0; i<array_size; ++i)
        blit (ar, obj[i]);
    ar.end_area (ci, (uintptr_t)aa);
    gather_logf_ident (-1, 0);
}

template<class ObjectT>
inline void blit_impl (blit_section& ar, ptr_address<ObjectT> objaddr, unsigned version, ptr_address_tag)
{
    ObjectT** oa = reinterpret_cast<ObjectT**> (objaddr.addr);

    typedef typename clean<typename std::remove_all_extents<ObjectT>::type>::type pointee_t;
    typedef typename clean<ObjectT>::type* pointer_t;

    gather_logf_ident (0, "pointer (%p x %lu @ %p), type: %s, pointee: %s\n",
        *oa, objaddr.count, objaddr.addr, shorten_name(typeid(pointer_t).name()).c_str(),
        typeid(pointee_t).name ()
    );

    blit_section::class_info* ptr_ci = ar.template get_class_info<pointer_t>();
//	blit_section::class_info* ci = ar.template get_class_info<pointee_t>();
    gather_logf_ident (1, 0);

    ar.begin_area (ptr_ci, (uintptr_t)oa, sizeof (pointer_t), 1, k_pointer, false);

    bool already_registered = ar.has_registered (*oa);

    if (already_registered)
        gather_logf_ident (0, "Object/ptr already registered\n");

    typedef typename boost::mpl::bool_<is_primitive<ObjectT>::value>::type is_prim_t;

    // process class only if pointer is valid and not registered yet
    if (!already_registered && *oa)
    {
        blit_section::class_info* ci = ar.template get_class_info<pointee_t>();
        category pointee_cat = type_category<pointee_t>::value ;

        gather_logf_ident (1, 0);
        // if the count if more that one, we need to put create an array 'container' to assure
        // that objects are serialized sequentially
        ar.begin_area (ci, (uintptr_t)*oa, sizeof (pointee_t), objaddr.count, pointee_cat, true);

        for (size_t i=0; i<objaddr.count; ++i)
            blit (ar, (*oa) [i]);

        //blit_array_helper (ar, *ppobj, objaddr.count, version, is_prim_t ());

        ar.end_area (ci, (uintptr_t)*oa);

        gather_logf_ident (-1, 0);
    }

    // we need to close pointer later to distinguish when a child object is inside a class or not
    ar.end_area (ptr_ci, (uintptr_t)oa);

    gather_logf_ident (-1, 0);
}

template<class ObjectT>
inline void blit_impl (blit_section& ar, ObjectT& obj, unsigned version, pointer_tag)
{
    ObjectT* po = (ObjectT*)pulmotor_addressof(obj);
    ptr_address<typename std::remove_pointer<ObjectT>::type> adr(po, 1);
    blit_impl (ar, adr, version, ptr_address_tag ());
}

// examine obj and forward serialization to the appropriate function by selecting the right tag
template<class ObjectT>
inline void blit_redirect (blit_section& ar, ObjectT& obj, unsigned flags_version)
{
    using boost::mpl::identity;
    using boost::mpl::eval_if;

    typedef typename std::remove_cv<ObjectT>::type actual_t;
    typedef typename boost::mpl::eval_if<
        is_primitive<actual_t>,
            identity<primitive_tag>,
            eval_if<
                std::is_array<actual_t>,
                identity<array_tag>,
                eval_if<
                    is_ptr_address<actual_t>,
                    identity<ptr_address_tag>,
                    eval_if<
                        is_array_address<actual_t>,
                        identity<array_address_tag>,
                        eval_if<
                            std::is_pointer<actual_t>,
                            identity<pointer_tag>,
                            identity<compound_tag>
                        >
                    >
                >
            >
    >::type tag_t;
    blit_impl (ar, obj, flags_version, tag_t());
}

template<class ObjectT>
inline blit_section& blit (blit_section& ar, ObjectT& obj)
{
    unsigned const ver = get_version<ObjectT>::value;
    blit_redirect (ar, obj, ver);
    return ar;
}

template<class ObjectT>
inline blit_section& operator & (blit_section& ar, ObjectT const& obj)
{
    return blit (ar, const_cast<ObjectT&>(obj));
}

template<class ObjectT>
inline blit_section& operator | (blit_section& ar, ObjectT const& obj)
{
    return blit (ar, const_cast<ObjectT&>(obj));
}

namespace util
{

template<class T>
inline void blit_to_container (T& a, std::vector<unsigned char>& odata, target_traits const& tt)
{
    pulmotor::blit_section bs;
    bs | a;
    bs.write_out (odata, tt, 4);
}

void* fixup_pointers_impl (pulmotor::blit_section_info* bsi);

template<class T>
inline T* fixup_pointers (pulmotor::blit_section_info* bsi)
{
    return (T*)fixup_pointers_impl (bsi);
}

template<class T>
inline T* fixup_pointers (void* dataWithHeader)
{
    blit_section_info* bsi = get_bsi (dataWithHeader, true);
    return (T*)fixup_pointers_impl (bsi);
}


inline void fixup (pulmotor::blit_section_info* bsi)
{
    char* data = ((char*)bsi + bsi->data_offset);
    uintptr_t* fixups = (uintptr_t*)((char*)bsi + bsi->fixup_offset);
    util::fixup_pointers (data, fixups, bsi->fixup_count);
}

template<class T>
size_t write_file (pp_char const* name, T& root, target_traits const& tt, size_t sectionalign)
{
    blit_section bs;

    bs | root;

    std::vector<pulmotor::u8> buffer;
    bs.write_out (buffer, tt, sectionalign);

    return write_file (name, &buffer[0], buffer.size ());
}

}

} // pulmotor

#endif
