#include <pulmotor/ser.hpp>
#include <pulmotor/stream.hpp>
#include <string>

namespace penis {

struct B
{
    float	c;
//	int*	i;
};

struct C
{
    unsigned c;
//	int*	i;
};

//namespace pulmotor {
    template<class ArchiveT>
    void serialize (ArchiveT& ar, B& obj, unsigned version) {
        using pulmotor::ptr;
        ar
//			| obj.i
            | obj.c
            ;
    }

    template<class ArchiveT>
    void serialize (ArchiveT& ar, C& obj, unsigned version) {
        using pulmotor::ptr;
        ar
//			| obj.i
            | obj.c
            ;
    }
//}
} // penis


    template<class ArchiveT>
    inline void serialize (ArchiveT& ar, std::string& s, unsigned version) {
        pulmotor::logf (" >>> serialize std::string: '%s'\n", s.c_str ());
    }

template<class T>
struct uninitialized
{
    explicit uninitialized (T* p) : p_(p) {}
    T*	p_;
};

namespace pulmotor {
    template<class ArchiveT>
    inline void serialize (ArchiveT& ar, std::string& s, unsigned version) {
        pulmotor::logf (" >>> blit std::string: '%s'\n", s.c_str ());
    }

/*	template<class ArchiveT, class CharT, class TraitsT, class AllocatorT>
    inline void load (ArchiveT& ar, uinitialized<std::basic_string<CharT, TraitsT, AllocatorT> > s, unsigned version)
    {
        typedef std::basic_string<CharT, TraitsT, AllocatorT> string_t;
        size_t sz;
        ar >> sz;

        new (s.p_) string_t ();
        s.p_->reserve (sz);
        ArchiveT::range strin = ar.acquire_block (sz);
        s.p_->assign (strin.begin (), strin.end ());
    }*/
}

struct A
{
    int	a, b, c;
    penis::B*	pb;
    penis::C	vc [2];

    std::string*	ss;
    std::string*	s2;
    std::string		ls;
    std::string**	aa;

    int	a1[5];

    A (std::string& str)
        :	ss(&str)
        ,	ls("local-string")
    {
        a = 0xaaaaaaa0;
        b = 0xbbbbbbb0;
        c = 0xccccccc0;

        pb = new penis::B;
        pb->c = 1;

        s2 = new std::string [5];
        s2[0] = "str-0";
        s2[1] = "str-1";
        s2[2] = "str-2";
        s2[3] = "str-3";
        s2[4] = "str-4";

        aa = new std::string* [2];
        aa[0] = new std::string ("---1+");
        aa[1] = new std::string ("---2+");

        vc[0].c = 0x77777777;
        vc[1].c = 0x88888888;
    }

    template<class ArchiveT>
    void serialize (ArchiveT& ar, unsigned version) {
        using pulmotor::ptr;
        using pulmotor::array;

        ar
            | ptr(aa,2)
            | pb
            | vc
            | ss
            | ptr(s2, 5)
            //| a1
            //| ls
            | a
            | c
            | b
            ;
    }
};

void moO (int (*p) [5]) { }
void moO (int const (*p) [5]) { }

int main ()
{
    int mm[5];
    moO (&mm);

    using namespace pulmotor;

    blit_section bs;

    std::string s ("orl");

    A a (s);
    A* pa = &a;
    (void)pa;

    bs | a;
    bs.dump_gathered ();

    //
    std::vector<unsigned char> buffer;

    {
        std::auto_ptr<basic_output_buffer> po = create_plain_output (pulmotor_native_path("ser_test.pulmotor"));
        size_t written = 0;
        po->write (&*buffer.begin (), buffer.size (), &written);
    }

    {
        std::auto_ptr<basic_input_buffer> pi = create_plain_input (pulmotor_native_path("ser_test.pulmotor"));
        std::vector<unsigned char> buffer;
        buffer.resize (1024 * 1024);

        size_t read = 0;
        pi->read (&*buffer.begin (), 1024 * 1024, &read);
        pulmotor::logf ("read: %d bytes\n", read);

        //
        pulmotor::blit_section_info* bsi = (pulmotor::blit_section_info*) (&*buffer.begin () + sizeof (pulmotor::basic_header));

        char* data = ((char*)bsi + bsi->data_offset);
        uintptr_t* fixups = (uintptr_t*)((char*)bsi + bsi->fixup_offset);

        pulmotor::util::fixup_pointers (data, fixups, bsi->fixup_count);

        A* a1 = (A*)data;
        (void)a1;
    }

    return 1;
}
