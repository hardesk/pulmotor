#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>
#include <pulmotor/archive.hpp>
#include <pulmotor/binary_archive.hpp>

using namespace pulmotor;
static romu3 r3;

struct A
{
    int a;
    long b;
    long long c;

    template<class ArchiveT>
    //void archive(ArchiveT& ar) { ar | mknv(a) | mknv(b) | mknv(c); }
    void archive(ArchiveT& ar, unsigned version) { ar | a | b | c; }
};

template<class T>
struct ar_check
{
    using v_t = std::vector<char>;

    static T one(v_t& v) {
        T a = *(T const*)&v[0];
        return a;
    }
    static T pull(v_t& v, size_t& offset) {
        T a = *(T const*)&v[offset];
        offset += sizeof(T);
        return a;
    }
    static std::string pull_str(v_t& v, size_t& offset) {
        T l = *(T const*)&v[offset];
        offset += sizeof(l);

        std::string ss(&v[offset], l);
        offset += l;
        return ss;
    }
};

/*
template<class... Archs>
struct multi_archive_out : public archive, archive_write_util<multi_archive_out<Archs...>>
{
    std::tuple<Archs&...> m_archs;
    fs_t m_offset = 0;

    multi_archive_out(unsigned flags, Archs&... archs) : m_archs(std::forward_as_tuple(archs...)), archive_write_util<multi_archive_out<Archs...>>(flags) {}
    multi_archive_out(multi_archive_out&&) = default;

    enum { is_reading = false, is_writing = true };
    fs_t offset() const { return m_offset; }

    template<class T>
    void write_single(T const& a) {
        util::map( [a] (auto& ar) { ar.write_single(a); }, m_archs);
        m_offset += sizeof a;
    }

    void write_data(void const* src, size_t size) {
        util::map( [src, size] (auto& ar) { ar.write_data(src, size); }, m_archs);
        m_offset += size;
    }

    void align_stream(size_t al) {
        util::map( [al] (auto& ar) { ar.align_stream(al); }, m_archs);
        if ((m_offset & (al-1)) != 0)
            m_offset = util::align(m_offset, al);
    }
};

template<class... Archs>
struct multi_archive_in : public archive
{
    std::tuple<Archs&...> m_archs;
    fs_t m_offset = 0;

    multi_archive_in(Archs&... archs) : m_archs(archs...) {}
    multi_archive_in(multi_archive_in<Archs...>&&) = default;

    // INTERFACE
    enum { is_reading = true, is_writing = false };
    fs_t offset() const { return m_offset; }

    template<class T>
    void read_basic(T& a) {
        T aa[sizeof...(Archs)];
        size_t i=0;
        util::map( [&a, &aa, &i] (auto& ar) { ar.read_basic(aa[i++]); }, m_archs);
        for (size_t x=1; x<sizeof...(Archs); ++x)
            CHECK_EQ(aa[0], aa[x]);
        a = aa[0];
        m_offset += sizeof a;
    }

    void read_data(void* dest, size_t size) {
        std::vector<char> aa[sizeof...(Archs)];
        size_t i=0;
        util::map( [dest, size, &aa, &i] (auto& ar) {
            ar.read_data(dest, size);
            aa[i].insert(aa[i].end(), (char*)dest, size);
        }, m_archs);

        for (size_t x=1; x<sizeof...(Archs); ++x)
            CHECK_EQ(aa[0], aa[x]);

        m_offset += size;
    }
};


struct multi_state
{
    struct write_state {
        std::stringstream ss;
        sink_ostream ssink;
        archive_sink ar_sink;
        archive_vector_out ar_vec;
        write_state() : ssink(ss), ar_sink(ssink), ar_vec() {}
    };

    struct read_state {
        std::stringstream ss1, ss2;
        std::unique_ptr<source_istream> s_is;
        std::unique_ptr<source_buffer> s_buff[2];

        std::unique_ptr<archive_whole> ar_whole;
        std::unique_ptr<archive_chunked> ar_chunked;
        std::unique_ptr<archive_chunked> ar_istream_source;
        std::unique_ptr<archive_istream> ar_istream;

        read_state(std::istream& is, char const* ptr, size_t len)
        {
            ss1 << is.rdbuf();
            ss2.str(std::string(ptr, len));

            s_is = std::make_unique<source_istream>(ss1);
            s_buff[0] = std::make_unique<source_buffer>(ptr, len);
            s_buff[1] = std::make_unique<source_buffer>(ptr, len);

            ar_whole = std::make_unique<archive_whole>(*s_buff[0]);
            ar_chunked = std::make_unique<archive_chunked>(*s_buff[1]);
            ar_istream_source = std::make_unique<archive_chunked>(*s_is);
            ar_istream = std::make_unique<archive_istream>(ss2);
        }
    };

    std::unique_ptr<write_state> ws_;
    std::unique_ptr<read_state> rs_;

    using out_arch_t = multi_archive_out<archive_sink, archive_vector_out>;
    using in_arch_t = multi_archive_in<archive_whole, archive_chunked, archive_chunked, archive_istream>;

    out_arch_t begin_write() {
        ws_ = std::move(std::make_unique<write_state>());
        rs_.reset();
        return out_arch_t(0, ws_->ar_sink, ws_->ar_vec);
    }

    in_arch_t read_written1() {
        rs_ = std::make_unique<read_state>(ws_->ss, ws_->ar_vec.data.data(), ws_->ar_vec.data.size());
        return in_arch_t(*rs_->ar_whole, *rs_->ar_chunked, *rs_->ar_istream_source, *rs_->ar_istream);
    }
};
*/
template<class Tuple, size_t... Is>
void print_tuple_impl( Tuple&& a, std::index_sequence<Is...> )
{
    ((std::cout << " " << std::get<Is>(a)), ...);
    std::cout << "\n";
}

template<class... Args>
void print_tuple( std::tuple<Args...>&& a )
{
    print_tuple_impl( a, std::make_index_sequence<sizeof...(Args)>() );
}

void serialize_test1(pulmotor::archive_sink& ar, std::string const& s);

TEST_CASE("pulmotor archive")
{
    pulmotor::sink_ostream ss(std::cout);
    pulmotor::archive_sink as(ss);
    serialize_test1(as, "hello");

    SUBCASE("print tuple")
    {
        print_tuple( std::tuple<int>{ 10 } );
        print_tuple( std::tuple<int, std::string>{ 10, "hello" } );
        print_tuple( std::tuple<int, char const*, std::string, long>{ 10, "hello", "world", 100 } );
    }

    SUBCASE("write basic")
    {
        using namespace pulmotor;
        SUBCASE("vector out") {
            archive_vector_out ar;
            ar.write_single('A');
            ar.write_single('X');
            CHECK(ar.str() == "AX");
        }

        SUBCASE("sink stream") {
            std::stringstream ss;
            sink_ostream s(ss);
            archive_sink ar(s);
            ar.write_single('A');
            ar.write_single('X');
            CHECK(ss.str() == "AX");
        }

        SUBCASE("stream out") {
            std::stringstream ss;
            sink_ostream s(ss);
            archive_sink ar(s);
            ar.write_single('A');
            ar.write_single('X');
            CHECK(ss.str() == "AX");
        }
    }

    SUBCASE("read basic")
    {
        using namespace pulmotor;

        char DAT[2] = { 'A', 'X' };

        SUBCASE("whole")
        {
            source_buffer sb(DAT, 2);
            archive_whole ar(sb);
            char a, x;
            ar.read_basic(a);
            ar.read_basic(x);
            CHECK(a == DAT[0]);
            CHECK(x == DAT[1]);
        }

        SUBCASE("chunked")
        {
            source_buffer sb(DAT, 2);
            archive_chunked ar(sb);
            char a, x;
            ar.read_basic(a);
            ar.read_basic(x);
            CHECK(a == DAT[0]);
            CHECK(x == DAT[1]);
        }

        SUBCASE("stream")
        {
            std::stringstream ss(std::string(DAT, 2));
            source_istream si(ss);
            archive_chunked ar(si);
            char a, x;
            ar.read_basic(a);
            ar.read_basic(x);
            CHECK(a == DAT[0]);
            CHECK(x == DAT[1]);
        }
    }

/*
    SUBCASE("multi write")
    {
        multi_state ms;

        multi_state::out_arch_t ar = ms.begin_write();
        ar.write_single((unsigned char)'A');
        ar.write_single((unsigned char)'X');

        multi_state::in_arch_t ia = ms.read_written1();
        unsigned char a, x;
        ia.read_basic(a);
        ia.read_basic(x);

        CHECK(a == 'A');
        CHECK(x == 'X');
    }*/

    SUBCASE("write block")
    {
        std::stringstream ss;
        pulmotor::sink_ostream so(ss);
        pulmotor::archive_sink ar(so);

        char data[]="hello";
        ar.write_data(data, strlen(data));
        CHECK(!ar.ec);

        CHECK(ss.str() == data);
    }

    SUBCASE("version")
    {
        struct A { int x; };
        A a{10};

        SUBCASE("basic")
        {
            pulmotor::archive_vector_out ar;
            ar.write_object_prefix(&a, 1337);
            CHECK(ar_check<u32>::one(ar.data) == 1337);
        }

        SUBCASE("typename")
        {
            pulmotor::archive_vector_out ar(ver_flag_debug_string);
            ar.write_object_prefix(&a, 1337);

            size_t offset = 0;
            CHECK((ar_check<u32>::pull(ar.data, offset) & ver_mask) == 1337);
            CHECK(ar_check<u32>::pull_str(ar.data, offset) == typeid(A).name());
        }

        size_t al = archive_vector_out::forced_align;
        SUBCASE("forced align")
        {
            pulmotor::archive_vector_out ar(ver_flag_align_object);
            ar.write_object_prefix(&a, 1337);
            CHECK(ar.offset() == al);

            size_t offset = 0;
            CHECK((ar_check<u32>::pull(ar.data, offset) & ver_mask) == 1337);
            size_t garbage = ar_check<u32>::pull(ar.data, offset);
            CHECK( (offset + garbage) == al);
        }

        SUBCASE("forced string+align")
        {
            pulmotor::archive_vector_out ar(ver_flag_debug_string|ver_flag_align_object);
            ar.write_object_prefix(&a, 1337);
            CHECK(ar.offset() == al);

            size_t offset = 0;
            CHECK((ar_check<u32>::pull(ar.data, offset) & ver_mask) == 1337);
            size_t garbage = ar_check<u32>::pull(ar.data, offset);
            CHECK(offset + garbage == al);
            CHECK(ar_check<u32>::pull_str(ar.data, offset) == typeid(A).name());
        }
    }
}

TEST_CASE("pulmotor archive version")
{
}

TEST_CASE("size packing")
{
    auto TC=[](int N, auto* pd) {
        int r=0;
        size_t s = 0;
        int state =0, i=0;

        while(i<12) {
            bool cont = util::duleb(s, state, pd[i++]);
            //CHECK(cont == (i<N));
            if (!cont) break;
        }
        CHECK(i==N);
        return s;
    };

    SUBCASE("u8") {
        u8 b[12];
        CHECK(util::euleb(0, b) == 1);
        CHECK(b[0]==0);
        CHECK(TC(1, b) == 0);

        CHECK(util::euleb(1, b) == 1);
        CHECK(b[0]==1);
        CHECK(TC(1, b) == 1);

        CHECK(util::euleb(128, b) == 2);
        CHECK(b[0]==0x80);
        CHECK(b[1]==0x01);
        CHECK(TC(2, b) == 128);

        // 7fff ->  0111 1111    1111 1111
        //		    0111 1111 1  M111 1111
        //		   01 M111 1111  M111 1111
        //		M..01 M111 1111  M111 1111
        CHECK(util::euleb(32767, b) == 3);
        CHECK(b[0]==0xff);
        CHECK(b[1]==0xff);
        CHECK(b[2]==0x01);
        CHECK(TC(3, b) == 32767);
    }

    SUBCASE("u16") {
        u16 b[12];

        CHECK(util::euleb(1, b) == 1);
        CHECK(TC(1, b) == 1);

        CHECK(util::euleb(0x7fff, b) == 1);
        CHECK(b[0]==0x7fff);
        CHECK(TC(1, b) == 0x7fff);

        CHECK(util::euleb(0x8000, b) == 2);
        CHECK(b[0]==0x8000);
        CHECK(b[1]==0x0001);
        CHECK(TC(2, b) == 0x8000);

        CHECK(util::euleb(0x82345678, b) == 3);
        CHECK(TC(3, b) == 0x82345678);
    }

    SUBCASE("u32") {
        u32 b[4];

        CHECK(util::euleb(1, b) == 1);
        CHECK(TC(1, b) == 1);

        CHECK(util::euleb(0x7fff'ffff, b) == 1);
        CHECK(b[0]==0x7fff'ffff);
        CHECK(TC(1, b) == 0x7fff'ffff);

        CHECK(util::euleb(0x8000'ffff, b) == 2);
        CHECK(b[0]==0x8000'ffff);
        CHECK(b[1]==0x0001);
        CHECK(TC(2, b) == 0x8000'ffff);

        size_t ss = 0xfedc'ba98'7654'3210u;
        CHECK(util::euleb(ss, b) == 3);
        CHECK(TC(3, b) == ss);
    }
}

namespace
{
    struct set_one : mod_op<set_one> {
        template<class Ar, class Ctx> void operator()(Ar& ar, Ctx& ctx) {
            ctx.one = 1;
        }
    };
    struct set_value : mod_op<set_value> {
        unsigned m_value;
        set_value(unsigned v) : m_value(v) {}
        set_value(set_value const&) = default;
        template<class Ar, class Ctx> void operator()(Ar& ar, Ctx& ctx) {
            ctx.value = m_value;
        }
    };
    struct arch0 {
    };
    struct arch1 {
        struct context { unsigned one = 0; unsigned value = 0; };
        using supported_mods = tl::list<set_one>;
    };
    struct arch2 {
        struct context { unsigned one = 0; unsigned value = 0; };
        using supported_mods = tl::list<set_one, set_value>;
    };
    struct arch3 {
        struct supports_kv {};
        struct context { char const* key; unsigned one = 0; unsigned value = 0; };
        using supported_mods = tl::list<set_one, set_value>;
    };
}

TEST_CASE("mods test")
{
    SUBCASE("type check")
    {
        CHECK(has_supported_mods<arch0>::value == false);
        CHECK(has_supported_mods<arch1>::value == true);
        CHECK(has_supported_mods<arch2>::value == true);

        CHECK(supports_kv<arch2>::value == false);
        CHECK(supports_kv<arch3>::value == true);
    }

    SUBCASE("none")
    {
        auto ml = 10 * set_one();
        arch0 a0;
        apply_mod_list(a0, a0, ml);
    }

    SUBCASE("applies")
    {
        auto ml = 10 * set_one() * set_value(2);
        ar | x * encbase64(enc::array);
        SUBCASE("1")
        {
            arch1 a;
            arch1::context ctx;
            apply_mod_list(a, ctx, ml);
            CHECK(ctx.one == 1);
            CHECK(ctx.value == 0);
        }

        SUBCASE("2")
        {
            arch2 a;
            arch2::context ctx;
            apply_mod_list(a, ctx, ml);
            CHECK(ctx.one == 1);
            CHECK(ctx.value == 2);
        }
    }

}
