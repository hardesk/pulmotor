#include <pulmotor/ser.hpp>
#include <pulmotor/stream.hpp>
#include <pulmotor/util.hpp>
#include <string>
#include <cstdio>

namespace penis {

struct A
{
    float	value;

    template<class ArchiveT>
    void serialize (ArchiveT& ar, unsigned version) {
        ar | value;
    }
};

struct B
{
    A	a;

    template<class ArchiveT>
    void serialize (ArchiveT& ar, unsigned version) {
        ar | a;
    }
};

struct C
{
    B*	pb;
    B	b;

    C ()
    {
        pb = new B;
        pb->a.value = 100;

        b.a.value = 200;
    }

    template<class ArchiveT>
    void serialize(ArchiveT& ar, unsigned version) {
        ar | pb | b;
    }
};

struct D
{
    C*	pc;
    B	b;
    C	c;
    A	a;

    D()
    {
        pc = new C;
    }

    template<class ArchiveT>
    void serialize (ArchiveT& ar, unsigned version) {
        ar | pc | b | c | a;
    }
};

} // penis


int main ()
{
    using namespace pulmotor;

    logf_ident (1, "1:0\n");
    logf_ident (1, "1:1\n");
    logf_ident (1, "1:2\n");
    logf_ident (0, "0:1\n");
    logf_ident (0, "0:2\n");
    logf_ident (-1, "-1:0\n");
    logf_ident (0, "0:0\n");
    logf_ident (-1, "-1:0\n");
    logf_ident (-1, "-1:1\n");
    logf_ident (0, "0:0\n");

    blit_section bs;

    penis::D data;
    bs | data;
    bs.dump_gathered ();

    //
    std::vector<unsigned char> buffer;

    {
        pulmotor::util::blit_to_container (data, buffer, target_traits::current);

        std::auto_ptr<basic_output_buffer> po = create_plain_output (pulmotor_native_path(".pulmotor"));
        size_t written = 0;
        po->write (&*buffer.begin (), buffer.size (), &written);

        penis::D* rd = pulmotor::util::fixup_pointers<penis::D> (pulmotor::util::get_bsi (&*buffer.begin (), false));
        printf ("rd: %p\n", rd);
    }

    return 1;
}
