#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>
#include <pulmotor/serialize.hpp>
#include <pulmotor/std/utility.hpp>
// #include <pulmotor/yaml_archive.hpp>
#include <pulmotor/binary_archive.hpp>
#include <span>

#include "common_formatter.hpp"
#include "common_types.hpp"
#include "common_archive.hpp"


namespace test_types
{
    struct DB { int x; };
    template<class Ar> void serialize(Ar& ar, DB& o) { ar | o.x; }

    struct DBM { int x; template<class Ar> void serialize(Ar& ar) { ar | x; } };

    struct DBSL { int x; };
    template<class Ar> void serialize_load(Ar& ar, DBSL& o) { ar | o.x; }
    template<class Ar> void serialize_save(Ar& ar, DBSL& o) { ar | o.x; }

    struct DD : DB			{ int y; template<class Ar> void serialize(Ar& ar) { ar | pulmotor::base<DB>(this) | y; } };
    struct DDM : DBM		{ int y; template<class Ar> void serialize(Ar& ar) { ar | pulmotor::base<DBM>(this) | y; } };
    struct DDSL : DBSL		{ int y; template<class Ar> void serialize(Ar& ar) { ar | pulmotor::base<DBSL>(this) | y; } };
} // test_types

PULMOTOR_DECL_VERSION(test_types::An, pulmotor::no_prefix);
PULMOTOR_DECL_VERSION(test_types::A2, 2);

namespace detect_serialize
{

struct X1 { }; template<class Ar> void serialize(Ar& ar, X1& x) { }
struct X2 { template<class Ar> void serialize(Ar& ar) { } };
struct X3 { }; template<class Ar> void serialize(Ar& ar, X3& x, unsigned version) { }
struct X4 { template<class Ar> void serialize(Ar& ar, unsigned version) { } };

struct XL1 { }; template<class Ar> void serialize_load(Ar& ar, XL1& x) { }
struct XL2 { template<class Ar> void serialize_load(Ar& ar) { } };
struct XL3 { }; template<class Ar> void serialize_load(Ar& ar, XL3& x, unsigned version) { }
struct XL4 { template<class Ar> void serialize_load(Ar& ar, unsigned version) { } };

struct XS1 { }; template<class Ar> void serialize_save(Ar& ar, XS1& x) { }
struct XS2 { template<class Ar> void serialize_save(Ar& ar) { } };
struct XS3 { }; template<class Ar> void serialize_save(Ar& ar, XS3& x, unsigned version) { }
struct XS4 { template<class Ar> void serialize_save(Ar& ar, unsigned version) { } };

struct X5 { }; template<class Ar, class F> void load_construct(Ar& ar, pulmotor::ctor<X5, F> const& o, unsigned) { }
struct X6 { template<class Ar, class F> static void load_construct(Ar& ar, pulmotor::ctor<X6, F> const& o, unsigned version) { } };
struct X7 { }; template<class Ar> void save_construct(Ar& ar, X7& x, unsigned version) { }
struct X8 { template<class Ar> void save_construct(Ar& ar, unsigned version) { } };

enum {
    S = 1, SM = 2, SV = 4, SMV = 8,
    LC = 16, LCM = 32, SC = 64, SCM = 128,
    LCP = 256, LCMP = 512,
    SL = 1024, SLM =2048, SLV = 4096, SLMV = 8192,
    SS = 16384, SSM = 32768, SSV = 65536, SSMV = 131072
};

template<class T, unsigned CHECKS, unsigned Sn, unsigned SLn, unsigned SSn, unsigned LCn, unsigned SCn>
struct test_type
{
    static void check();
};

}

template<class Ar> void serialize(Ar& ar, detect_serialize::X1& x) { }

namespace detect_serialize {

template<class T, unsigned CHECKS, unsigned Sn, unsigned SLn, unsigned SSn, unsigned LCn, unsigned SCn>
void test_type<T,CHECKS,Sn,SLn,SSn,LCn,SCn>::check()
{
    using namespace pulmotor;
    using Ar = pulmotor::archive;

    auto f = [](auto&&...) {};
//	using F = decltype(f);
    CHECK(access::detect<Ar>::has_serialize<T>::value == ((CHECKS & S) != 0));
    CHECK(access::detect<Ar>::has_serialize_mem<T>::value == ((CHECKS & SM) != 0));
    CHECK(access::detect<Ar>::has_serialize_version<T>::value == ((CHECKS & SV) != 0));
    CHECK(access::detect<Ar>::has_serialize_mem_version<T>::value == ((CHECKS & SMV) != 0));

    CHECK(access::detect<Ar>::has_serialize_load<T>::value == ((CHECKS & SL) != 0));
    CHECK(access::detect<Ar>::has_serialize_load_mem<T>::value == ((CHECKS & SLM) != 0));
    CHECK(access::detect<Ar>::has_serialize_load_version<T>::value == ((CHECKS & SLV) != 0));
    CHECK(access::detect<Ar>::has_serialize_load_mem_version<T>::value == ((CHECKS & SLMV) != 0));

    CHECK(access::detect<Ar>::has_serialize_save<T>::value == ((CHECKS & SS) != 0));
    CHECK(access::detect<Ar>::has_serialize_save_mem<T>::value == ((CHECKS & SSM) != 0));
    CHECK(access::detect<Ar>::has_serialize_save_version<T>::value == ((CHECKS & SSV) != 0));
    CHECK(access::detect<Ar>::has_serialize_save_mem_version<T>::value == ((CHECKS & SSMV) != 0));

    CHECK(access::detect<Ar>::has_load_construct<T>::value == ((CHECKS & LC) != 0));
    CHECK(access::detect<Ar>::has_load_construct_mem<T>::value == ((CHECKS & LCM) != 0));
    CHECK(access::detect<Ar>::has_save_construct<T>::value == ((CHECKS & SC) != 0));
    CHECK(access::detect<Ar>::has_save_construct_mem<T>::value == ((CHECKS & SCM) != 0));

    //CHECK(access::detect<Ar>::count_serialize<T>::value == Sn);
    //CHECK(access::detect<Ar>::count_load_construct<T>::value == LCn);
    //CHECK(access::detect<Ar>::count_save_construct<T>::value == SCn);
}

} // detect_serialize

TEST_CASE("detect")
{
    using namespace detect_serialize;

    test_type<X1,  S  , 1, 0, 0, 0, 0>::check();
    test_type<X2,  SM , 1, 0, 0, 0, 0>::check();
    test_type<X3,  SV , 1, 0, 0, 0, 0>::check();
    test_type<X4,  SMV, 1, 0, 0, 0, 0>::check();

    test_type<XL1, SL , 0, 1, 0, 0, 0>::check();
    test_type<XL2, SLM, 0, 1, 0, 0, 0>::check();
    test_type<XL3, SLV, 0, 1, 0, 0, 0>::check();
    test_type<XL4, SLMV,0, 1, 0, 0, 0>::check();

    test_type<XS1, SS , 0, 0, 1, 0, 0>::check();
    test_type<XS2, SSM, 0, 0, 1, 0, 0>::check();
    test_type<XS3, SSV, 0, 0, 1, 0, 0>::check();
    test_type<XS4, SSMV,0, 0, 1, 0, 0>::check();

    test_type<X5, LC , 0, 0, 0, 1, 0>::check();
    test_type<X6, LCM, 0, 0, 0, 1, 0>::check();
    test_type<X7, SC , 0, 0, 0, 0, 1>::check();
    test_type<X8, SCM, 0, 0, 0, 0, 1>::check();
}

TEST_CASE("detect pair")
{
    using Ar = pulmotor::archive;
    CHECK(pulmotor::access::detect<Ar>::has_serialize<std::pair<int, int> >::value == true);
}

TEST_CASE("serialize binary specific")
{
    using namespace pulmotor;
    using namespace test_types;

    BinaryArchiveTest a;

    SUBCASE("alignment")
    {
        using namespace std::string_literals;
        using namespace pulmotor;

        u8 a8{0x38};
        u16 a16{0x3631};
        u32 a32{0x32333233};
        u64 a64{0x3436343634363436};

        u8 x8;
        u16 x16;
        u32 x32;
        u64 x64;

        //archive_vector_out ar;
        sink_vector out; archive_sink ar(out);

        SUBCASE("8") {
            a.o | a8 | a16 | a32 | a64;
            CHECK(pulmotor::util::hexv(a.data()) == "8\0" "16323264646464"_hexs);

            auto i = a.make_reader();
            i | x8 | x16 | x32 | x64;

            CHECK(a8==x8);
            CHECK(a16==x16);
            CHECK(a32==x32);
            CHECK(a64==x64);
        }

        SUBCASE("16") {
            a.o | a16 | a32 | a64;
            CHECK(pulmotor::util::hexv(a.data()) == "16\0\0" "323264646464"_hexs);

            auto i = a.make_reader();
            i | x16 | x32 | x64;
            CHECK(a16==x16);
            CHECK(a32==x32);
            CHECK(a64==x64);
        }

        SUBCASE("32") {
            a.o | a32 | a64;
            CHECK(pulmotor::util::hexv(a.data()) == "3232\0\0\0\0""64646464"_hexs);

            auto i = a.make_reader();
            i | x32 | x64;
            CHECK(a32==x32);
            CHECK(a64==x64);
        }

        SUBCASE("16\08") {
            a.o | a16 | a8 | a32 | a64;
            CHECK(pulmotor::util::hexv(a.data()) == "168\0""323264646464"_hexs);

            auto i = a.make_reader();
            CHECK(a16==x16);
            CHECK(a8==x8);
            CHECK(a32==x32);
            CHECK(a64==x64);
        }

        SUBCASE("16\032\08") {
            a.o | a16 | a32 | a8 | a64;
            CHECK(pulmotor::util::hexv(a.data()) == "16\0\0""32328\0\0\0\0\0\0\0""64646464"_hexs);

            auto i = a.make_reader();
            i | x16 | x32 | x8 | x64;
            CHECK(a16==x16);
            CHECK(a32==x32);
            CHECK(a8==x8);
            CHECK(a64==x64);
        }
    }

    SUBCASE("struct of older version")
    {
        std::vector<char> data1 = "\1\0\0\0\1\0\0\0\xd2\4\0\0"_v;
        std::vector<char> data3 = "\3\0\0\0\1\0\0\0\xd2\4\0\0\1\1\0\0"_v;

        archive_memory i1 = std::span<char const>(data1);
        A2 x;
        i1 | x;

        CHECK(x.y == 2);

        archive_memory i3 = std::span<char const>(data3);
        A2 x1;
        i3 | x1;

        CHECK(x1.y == 257);
    }

}

// template<class T, class ArchPair = YamlArchiveTest>
// struct basic_tester {
//     void operator() (T value) {
//         ArchPair a;
//         a.o | value;

//         // check binary it's the same written
//         if constexpr (std::is_same<decltype(a.make_reader()), pulmotor::archive_memory>::value) {
//             char buffer[sizeof(value)];
//             memcpy(buffer, a.data().data(), a.data().size());
//             CHECK(value == *reinterpret_cast<decltype(value)*>(buffer));
//         }

//         T v1;
//         auto i = a.make_reader();
//         i | v1;

//         CHECK( v1 == value );
//     }
// };

TEST_CASE_TEMPLATE("value stream serialize", ArchPair, YamlArchiveTest, BinaryArchiveTest)
{
    using namespace pulmotor;

    auto basic_test = []<class T>(T value) {
        ArchPair ap;
        ap.o | value;

        // check binary it's the same written
        if constexpr (std::is_same<decltype(ap.make_reader()), pulmotor::archive_memory>::value) {
            char buffer[sizeof(value)];
            memcpy(buffer, ap.data().data(), ap.data().size());
            CHECK(value == *reinterpret_cast<decltype(value)*>(buffer));
        }

        T v1;
        auto i = ap.make_reader();
        i | v1;

        CHECK( v1 == value );

        // std::cout << typeid(value).name() << std::endl;
    };

    SUBCASE("fundamental types")
    {
        basic_test((bool)false);
        basic_test((bool)true);
        basic_test((char)'a');
        basic_test((signed char)1);
        basic_test((unsigned char)250);
        basic_test((short)16384);
        basic_test((short)1);
        basic_test((unsigned short)65535);
        basic_test((int)100000);
        basic_test((int)100000);
        basic_test((unsigned)3002001000);
        basic_test((long)3002001000);
        basic_test((unsigned long)3002001000);
        basic_test((long long)3002001000);
        basic_test((unsigned long long)3002001000);

        basic_test((u8)0xff);
        basic_test((s8)0x7f);
        basic_test((s8)0x80);
        basic_test((u16)0xffff);
        basic_test((s16)0x7fff);
        basic_test((s16)0x8000);
        basic_test((u32)0xffff'ffffU);
        basic_test((s32)0x7fff'ffff);
        basic_test((s32)0x8000'0000);
        basic_test((u64)0xffff'ffff'ffff'ffff);
        basic_test((s64)0x7fff'ffff'ffff'ffff);
        basic_test((s64)0x8000'0000'0000'0000);
    }
}

// This supports only keyed serialization
TEST_CASE_TEMPLATE("named value serialize", ArchPair, YamlKeyedArchiveTest, YamlArchiveTest, BinaryArchiveTest)
{
    using namespace pulmotor;
    using namespace test_types;

    ArchPair ap;
    constexpr bool is_binary = std::is_same<decltype(ap.make_reader()), pulmotor::archive_memory>::value;
    SUBCASE("struct")
    {
        Ak s{1234};
        ap.o | s;

        if constexpr (is_binary)
            CHECK(pulmotor::util::hexv(ap.data()) == "\1\0\0\0\xd2\4\0\0"_hexs);

        auto i = ap.make_reader();
        Ak x { 0 };
        i | x;

        CHECK( s == x );
    }

    SUBCASE("primitive array")
    {
        int a[2] = {1, 120};
        ap.o | array(a, 2);

        // for natural align policy
        if constexpr (is_binary)
            CHECK(pulmotor::util::hexv(ap.data()) == "\2\0\0\0\1\0\0\0\x78\0\0\0"_hexs);

        auto i1 = ap.make_reader();
        int x1[2] = { 0, 0 };
        size_t s = 2;
        i1 | array_size(s);
        i1 | array_data(x1, s);
        CHECK( a[0] == x1[0] );
        CHECK( a[1] == x1[1] );

        auto i2 = ap.make_reader();
        int x2[2] = { 0, 0 };
        i2 | array_size(s);
        i2 | x2[0] | x2[1];
        i2.end_array();
        CHECK( a[0] == x2[0] );
        CHECK( a[1] == x2[1] );
    }

    SUBCASE("primitive array fixed size")
    {
        int a[2] = {1, 120};
        ap.o | a;

        if constexpr (is_binary)
            CHECK(pulmotor::util::hexv(ap.data()) == "\2\0\0\0\1\0\0\0\x78\0\0\0"_hexs);

        auto i = ap.make_reader();
        int x[2] = { 0, 0 };
        i | x;
        CHECK( a[0] == x[0] );
        CHECK( a[1] == x[1] );
    }
}

TEST_CASE_TEMPLATE("value serialize", ArchPair, YamlArchiveTest, BinaryArchiveTest)
{
    using namespace pulmotor;
    using namespace test_types;

    ArchPair ap;

    constexpr bool is_binary = std::is_same<decltype(ap.make_reader()), pulmotor::archive_memory>::value;

    SUBCASE("struct no prefix")
    {
        An s{1234};
        ap.o | s;

        if constexpr (is_binary)
            CHECK(pulmotor::util::hexv(ap.data()) == "\xd2\4\0\0"_hexs);

        auto i = ap.make_reader();
        An x { 0 };
        i | x;

        CHECK( s == x );
    }

    SUBCASE("array of struct")
    {
        A a[2] = {{1234}, {5678}};
        size_t s = 2;
        ap.o | array_size(s);
        ap.o | pulmotor::array_data(a, 2);

        // for natural align policy
        // if constexpr (is_binary)
        //     CHECK(pulmotor::util::hexv(ap.data()) == "\2\0\1\0\0\0\0\0\xd2\4\0\0\x2e\x16\0\0"_hexs);

        auto i = ap.make_reader();
        A x[2];
        i | array_size(s);
        i | pulmotor::array_data(x, 2);
        CHECK( a[0] == x[0] );
        CHECK( a[1] == x[1] );
    }

    SUBCASE("array of struct fixed size")
    {
        A a1[2] = {{1234}, {5678}};
        ap.o | a1;

        // if constexpr (is_binary)
        //     CHECK(pulmotor::util::hexv(ap.data()) == "\2\0\1\0\0\0\0\0\xd2\4\0\0\x2e\x16\0\0"_hexs);

        auto i1 = ap.make_reader();
        A x1[2];
        i1 | x1;
        CHECK( a1[0] == x1[0] );
        CHECK( a1[1] == x1[1] );
    }

    SUBCASE("array of array")
    {
        A a2[2][3] = { {{1234}, {5678}, {9112}}, {{1234}, {5678}, {9556}} };
        ap.o | a2;

        auto i2 = ap.make_reader();
        A x2[2][3];
        i2 | x2;
        CHECK( a2[0][0] == x2[0][0] );
        CHECK( a2[0][1] == x2[0][1] );
        CHECK( a2[0][2] == x2[0][2] );
        CHECK( a2[1][0] == x2[1][0] );
        CHECK( a2[1][1] == x2[1][1] );
        CHECK( a2[1][2] == x2[1][2] );
    }

    SUBCASE("nested struct 0")
    {
        N n { {1233} };
        ap.o | n;

        auto i = ap.make_reader();
        N n1;
        i | n1;
        CHECK( n1.a.x == n.a.x );
    }

    SUBCASE("nested struct 1")
    {
        A2 x{{1111}, 2222};
        ap.o | x;

        auto i = ap.make_reader();
        A2 x1;
        i | x1;
        CHECK( x.a == x1.a );
        CHECK( x.y == x1.y );
    }


    SUBCASE("base")
    {
        auto s = [&ap]<class T>(T& a)
        {
            ap.o | a;

            auto i = ap.make_reader();
            T x;
            i | x;
            CHECK(x.x == a.x);
            CHECK(x.y == a.y);
        };

        SUBCASE("1")
        {
            DD a{ {1}, 2};
            s(a);
        }

        SUBCASE("2")
        {
            DDM a{ {1}, 2};
            s(a);
        }
        SUBCASE("3")
        {
            DDSL a{ {1}, 2};
            s(a);
        }
    }


    SUBCASE("variable unsigned quantity")
    {
        size_t a=10, b=255, c=32768, d=4000000;
        SUBCASE("u8")
        {
            auto check = [&ap](size_t v, std::initializer_list<u8> _l) {
                ap.o | vu<u8>(v);
                u8 const* l = _l.begin();
                if constexpr (is_binary) {
                    if (_l.size() > 0) CHECK((u8)ap.data()[0] == l[0]);
                    if (_l.size() > 1) CHECK((u8)ap.data()[1] == l[1]);
                    if (_l.size() > 2) CHECK((u8)ap.data()[2] == l[2]);
                    if (_l.size() > 3) CHECK((u8)ap.data()[3] == l[3]);
                }

                size_t x = 0;
                auto i = ap.make_reader();
                i | vu<u8>(x);
                CHECK(v == x);
            };
            // a=32768; while [[ $a -ne 0 ]]; do if [[ $a -gt 127 ]]; then echo $((a & 0x7f|0x80)); else echo $((a & 0x7f)); fi; a=$((a>>7)); done;
            SUBCASE("1") {
                check(a, {10});
            }
            SUBCASE("2") {
                check(b, {0xff, 0x01});
            }
            SUBCASE("3") {
                check(c, {0x80, 0x80, 0x02});
            }
            SUBCASE("4") {
                check(d, {0x80, 0x92, 0xf4, 0x01 });
            }
        }

        SUBCASE("u16")
        {
            auto check = [&ap](size_t v, std::initializer_list<u8> _l) {
                ap.o | vu<u16>(v);
                u8 const* l = _l.begin();
                if constexpr (is_binary) {
                    if (_l.size() > 0) CHECK((u8)ap.data()[0] == l[0]);
                    if (_l.size() > 1) CHECK((u8)ap.data()[1] == l[1]);
                    if (_l.size() > 2) CHECK((u8)ap.data()[2] == l[2]);
                    if (_l.size() > 3) CHECK((u8)ap.data()[3] == l[3]);
                }

                size_t x = 0;
                auto i = ap.make_reader();
                i | vu<u16>(x);
                CHECK(v == x);
            };

            // a=32768; while [[ $a -ne 0 ]]; do if [[ $a -gt 32767 ]]; then echo $((a & 0x7fff|0x8000)); else echo $((a & 0x7fff)); fi; a=$((a>>15)); done;
            SUBCASE("1") {
                check(a, { 10, 0 });
            }
            SUBCASE("2") {
                check(b, { 0xff, 0x00 });
            }
            SUBCASE("3") {
                check(c, { 0x00, 0x80, 0x01, 0x00 });
            }
            SUBCASE("4") {
                check(d, { 0x00, 0x89, 0x7a, 0x00 });
            }
        }
    }
}

namespace enum_types
{
    enum A { A0 = 0, A1 = 200 };
    enum S : short { S0 = -1, S1 = 40 };
    enum class C { C0 = 2 };
    enum class X : uint8_t { X0 = 102 };
}

TEST_CASE_TEMPLATE("enum serialize", ArchPair, YamlKeyedArchiveTest, YamlArchiveTest, BinaryArchiveTest)
{
    using namespace enum_types;
    using namespace pulmotor;
    ArchPair ap;

    A a{A1}, a1;
    S s{S0}, s1;
    C c{C::C0}, c1;
    X x{X::X0}, x1;

    ap.o.begin_array(4);
    ap.o | a | s | c | x;
    ap.o.end_array();

    auto i = ap.make_reader();
    i.begin_array(4);
    i | a1 | s1 | c1 | x1;
    i.end_array();

    CHECK(a1 == a);
    CHECK(s1 == s);
    CHECK(c1 == c);
    CHECK(x1 == x);
}

#if 0
TEST_CASE("ptr serialize")
{
    using namespace ptr_types;
    using namespace pulmotor;
    archive_vector_out ar;

    SUBCASE("serialize nullptr")
    {
        Y y0, y1;
        ar | y0 | y1;

        Y z0, z1;

        // serializing nullptr works
        archive_vector_in i(ar.data);
        i | z0;
        CHECK(z0.px == nullptr);

        // deserializing nullptr over ptr nullifies pointer
        z1.init(-1);
        i | z1;
        CHECK(z1.px == nullptr);
    }

    SUBCASE("serialize pointer")
    {
        Y y1, y2, y3;
        y1.init(0x11111111);
        y2.init(0x22222222);
        y3.init(0x33333333);
        ar | y1 | y2 | y3;

        Y z1, z2;
        archive_vector_in i(ar.data);

        i | z1; // deserialize onto null ptr
        CHECK(z1.px != nullptr);
        CHECK(z1.px != y1.px);
        CHECK(z1.px->x == y1.px->x);

        z2.init(0xaaaaaaaa);
        i | z2; // deserialize ptr over existing ptr
        CHECK(z2.px != y2.px);
        CHECK(z2.px != nullptr);
        CHECK(z2.px->x == z2.px->x);

        i | z2; // deserializing over exising (deserialized) ptr
        CHECK(z2.px != nullptr);
        CHECK(z2.px->x == y3.px->x);
    }

    SUBCASE("load construct")
    {
        A* a = new A(100);
        ar | a;

        A* pa = nullptr;
        archive_vector_in i(ar.data);
        i | pa;

        CHECK(a->x == pa->x);
        delete pa;

        delete a;
    }

    SUBCASE("user alloc")
    {
        std::set<void*> ps;
        auto ma = [&ps] () { void* p = malloc(sizeof(A)); ps.insert(p); return p; };
        auto da = [&ps] (void* p) { CHECK(ps.count(p) == 1); ps.erase(p); free(p); };
        auto s = [&] (auto& ar, A*& a) { ar | alloc(a, ma, da); };

        A* a = new (ma()) A(5555);
        s(ar, a);
        s(ar, a);

        archive_vector_in i(ar.data);
        A* aa = nullptr;
        s(i, aa);
        CHECK(aa != a);
        CHECK(a->x == aa->x);

        aa->x = 137;
        s(i, aa);
        CHECK(aa != a);
        CHECK(a->x == aa->x);

        da(a);
        da(aa);
    }

    SUBCASE("placement")
    {
        int x = 200, xx = 0;
        std::aligned_storage_t<sizeof(A)>::type a[1];
        new (a) A(12345678);
        ar | place<A>(a) | x;

        std::aligned_storage_t<sizeof(A)>::type aa[1];
        archive_vector_in i(ar.data);
        i | place<A>(aa) | xx;

        CHECK(x == xx);

        A& a1 = *reinterpret_cast<A*>(a);
        A& a2 = *reinterpret_cast<A*>(aa);

        CHECK(a1.x == a2.x);
    }

    SUBCASE("placement new")
    {
        std::aligned_storage_t<sizeof(A)> a[1];
        new (a) A(123456);
        ar | place<A>(a);

        P p1(100);
        ar | p1;

        std::aligned_storage_t<sizeof(A)> aa[1];
        archive_vector_in i(ar.data);
        i | place<A>(aa);

        A& a1 = *reinterpret_cast<A*>(a);
        A& a2 = *reinterpret_cast<A*>(aa);

        CHECK(a1.x == a2.x);

        P p2(200);
        i | p2;
        CHECK(p1.getA().x == p2.getA().x);

    }

    SUBCASE("with allocator and destroy and null")
    {
        Zb zb0, zb1(200);
        ar | zb0 | zb1;

        archive_vector_in i(ar.data);
        Zb zx0, zx1(400);
        i | zx0 | zx1;

        CHECK(zb0.px == nullptr);
        CHECK(zx0.px == nullptr);
        CHECK(zx1.px->x == zb1.px->x);
        CHECK(zx1.px != zb1.px);

        archive_vector_in i1(ar.data);
        Zb zy0(300), zy1;
        i1 | zy0 | zy1;

        CHECK(zy0.px == nullptr);
        CHECK(zy1.px != nullptr);
        CHECK(zy1.px->x == zb1.px->x);
    }

    SUBCASE("with allocator")
    {
        Za za(10);
        ar | za;

        archive_vector_in i(ar.data);
        Za zb(0);
        i | zb;

        CHECK(za.px->x == zb.px->x);
    }

    SUBCASE("emplace")
    {
        A* a = new A(10);
        ar | a;

        archive_vector_in i(ar.data);
        A* aa{nullptr};
        i | construct<A>(aa, [](int a) { return new A(a); }, [](A* p) { delete p; } );

        REQUIRE(aa != nullptr);
        CHECK(aa != a);
        CHECK(aa->x == a->x);

        bool did_delete = false;
        bool did_alloc = false;
        archive_vector_in i2(ar.data);
        i2 | construct<A>(aa, [&](int a) { did_alloc = true; return new A(a); }, [&](A* p) { did_delete = true; delete p; } );

        CHECK(did_alloc == true);
        CHECK(did_delete == true);
        CHECK(aa->x == a->x);

        delete a;
        delete aa;
    }

    SUBCASE("emplace_back")
    {
        A a{20};
        ar | a;

        archive_vector_in i(ar.data);
        std::vector<A> va;
        i | construct<A>([&va](int a) { va.emplace_back(a); } );

        REQUIRE(va.size() == 1);
        CHECK(va.back().x == a.x);
    }

#if 0
    /*SUBCASE("primitive compile speed test")
    {
        using namespace pulmotor;
#define S0(x) int x##_a=0; char x##_b=1; long x##_c=2; ar | x##_a | x##_b | x##_c;
#define S1(x) S0(x##0) S0(x##1) S0(x##2) S0(x##3) S0(x##4) S0(x##5) S0(x##6) S0(x##7)
#define S2(x) S1(x##0) S1(x##1) S1(x##2) S1(x##3) S1(x##4) S1(x##5) S1(x##6) S1(x##7)
#define S3(x) S2(x##0) S2(x##1) S2(x##2) S2(x##3) S2(x##4) S2(x##5) S2(x##6) S2(x##7)
#define S4(x) S3(x##0) S3(x##1) S3(x##2) S3(x##3) S3(x##4) S3(x##5) S3(x##6) S3(x##7)
#define S5(x) S4(x##0) S4(x##1) S4(x##2) S4(x##3) S4(x##4) S4(x##5) S4(x##6) S4(x##7)
//#define S6(x) S5(x##0) S5(x##1) S5(x##2) S5(x##3) S5(x##4) S5(x##5) S5(x##6) S5(x##7)

        std::stringstream ss;
        pulmotor::sink_ostream so(ss);
        pulmotor::sink_archive ar(so);

        S4(xx);
    }*/
#endif
}

namespace loadsave_types
{

    struct S { int x; };
    template<class Ar> void serialize_load(Ar& ar, S& x) { ar | x.x; }
    template<class Ar> void serialize_save(Ar& ar, S& x) { ar | x.x; }

    struct SM {
        int x;
        template<class Ar> void serialize_load(Ar& ar) { ar | x; }
        template<class Ar> void serialize_save(Ar& ar) { ar | x; }
    };

    struct SV { int x; };
    template<class Ar> void serialize_load(Ar& ar, SV& x, unsigned version) { ar | x.x; }
    template<class Ar> void serialize_save(Ar& ar, SV& x, unsigned version) { ar | x.x; }

    struct SMV {
        int x;
        template<class Ar> void serialize_load(Ar& ar, unsigned version) { ar | x; }
        template<class Ar> void serialize_save(Ar& ar, unsigned version) { ar | x; }
    };
}

TEST_CASE("load save variants")
{
    using namespace loadsave_types;
    using namespace pulmotor;
    pulmotor::archive_vector_out ar;

    auto p = [&ar](auto& x0, auto& x1, auto& z0, auto& z1)
    {
        ar | x0 | x1;

        archive_vector_in i(ar.data);
        i | z0 | z1;

        CHECK(z0.x == x0.x);
        CHECK(z1.x == x1.x);
    };

    SUBCASE("serialize_load/save")
    {
        S x0{0}, x1{100}, z0, z1;
        p(x0, x1, z0, z1);
    }

    SUBCASE("serialize_load/save member")
    {
        SM x0{0}, x1{100}, z0, z1;
        p(x0, x1, z0, z1);
    }

    SUBCASE("serialize_save/load version")
    {
        SV x0{0}, x1{100}, z0, z1;
        p(x0, x1, z0, z1);
    }

    SUBCASE("serialize_save/load member version")
    {
        SMV x0{0}, x1{100}, z0, z1;
        p(x0, x1, z0, z1);
    }
}

namespace ver_types {
    struct X { enum { version = 5 }; };
    struct Y {};
    struct Z {};

namespace v1 {
    struct A
    {
        int a = -1;
        template<class Ar> void serialize(Ar& ar) { ar | a; }
    };
}

namespace v2 {
    struct A
    {
        int a = -1;
        int b = -1;
        template<class Ar> void serialize(Ar& ar, unsigned version) {
            ar | a;
            if (version >= 2)
                ar | b;
        }
    };
}

namespace v3 {
    struct A
    {
        enum { version = 3 };

        int a;
        int b = -1;
        int c = -1;
        template<class Ar> void serialize(Ar& ar, unsigned version) {
            ar | a;
            if (version >= 2)
                ar | b;
            if (version >= 3)
                ar | c;
        }
    };
}
}

PULMOTOR_VERSION(ver_types::Z, 33);

template<> struct pulmotor::class_version<ver_types::v2::A> { static constexpr unsigned value = 2; };

TEST_CASE("version checks")
{
    SUBCASE("direct version check") {
        CHECK(pulmotor::get_meta<ver_types::X>::value == 5);
        CHECK(pulmotor::get_meta<ver_types::Y>::value == pulmotor::version_default);
        CHECK(pulmotor::get_meta<ver_types::Z>::value == 33);
    }

    pulmotor::archive_vector_out ar;

    SUBCASE("") {
        using namespace ver_types;

        v1::A a1{0x11};
        v2::A a2{0x21, 0x22};
        v3::A a3{0x31, 0x32, 0x33};

        ar | a1
            | a1 | a2
            | a1 | a2 | a3;

        pulmotor::archive_vector_in i(ar.data);
        v1::A x1;
        v2::A x21, x22;
        v3::A x31, x32, x33;
        i | x1
        | x21 | x22
        | x31 | x32 | x33;

        CHECK(x1.a == a1.a);

        CHECK(x21.a == a1.a);
        CHECK(x21.b == -1);
        CHECK(x22.a == a2.a);
        CHECK(x22.b == a2.b);

        CHECK(x31.a == a1.a);
        CHECK(x31.b == -1);
        CHECK(x31.c == -1);
        CHECK(x32.a == a2.a);
        CHECK(x32.b == a2.b);
        CHECK(x32.c == -1);
        CHECK(x33.a == a3.a);
        CHECK(x33.b == a3.b);
        CHECK(x33.c == a3.c);
    }
}


#include <pulmotor/std/map.hpp>
TEST_CASE("map")
{
    using namespace ptr_types;
    using namespace pulmotor;
    using namespace std::string_literals;

    archive_vector_out ar;

    SUBCASE("empty")
    {
        std::map<int, A> m;
        ar | m | m;

        std::map<int, A> x, y{ {10, A{11}}};
        archive_vector_in i(ar.data);
        i | x | y;
        CHECK(x.empty());
        CHECK(y.empty());
    }

    SUBCASE("int")
    {
        std::map<int, int> m{ {10, 1}, {20, 2}, {30, 3} };
        ar | m;

        archive_vector_in i(ar.data);
        std::map<int, int> x;
        i | x;

        CHECK(x.size() == m.size());
        CHECK(x == m);
    }

    SUBCASE("int ptr")
    {
        std::map<int, int*> m{ {10, new int(1)}, {20, new int(2)} };
        ar | m;

        archive_vector_in i(ar.data);
        std::map<int, int*> x;
        i | x;

        CHECK((std::equal(x.begin(), x.end(), m.begin(), m.end(), [](auto a, auto b) { return a.first == b.first && *a.second == *b.second; })) == true);
        CHECK(x.size() == m.size());
        for(auto z : m) delete z.second;
        for(auto z : x) delete z.second;
    }

    SUBCASE("struct")
    {
        std::map<std::string, X> m{ {"jon", X{10}}, {"susie", X{20}}, {"barnie", X{30}} };
        ar | m;

        archive_vector_in i(ar.data);
        std::map<std::string, X> x;
        i | x;

        CHECK(x.size() == m.size());
        CHECK(x == m);
    }

    SUBCASE("struct ctor")
    {
        std::map<std::string, A> m{ {"jon"s, A{10}}, {"susie"s, A{20}}, {"barnie"s, A{30}} };
        ar | m;

        archive_vector_in i(ar.data);
        std::map<std::string, A> x;
        i | x;

        CHECK(x.size() == m.size());
        CHECK(x == m);
    }

    SUBCASE("struct ptr ctor")
    {
        std::map<int, A*> m{ {10, new A{15}}, {30, new A{25}}};
        ar | m;

        archive_vector_in i(ar.data);
        std::map<int, A*> x;
        i | x;

        CHECK(x.size() == m.size());
        CHECK((std::equal(x.begin(), x.end(), m.begin(), m.end(), [](auto a, auto b) { return a.first == b.first && *a.second == *b.second; })) == true);
        for(auto z : m) delete z.second;
        for(auto z : x) delete z.second;
    }
}

TEST_CASE("yaml")
{
    SUBCASE("primitives")
    {
    }

    SUBCASE("struct")
    {
    }

    SUBCASE("primitive array")
    {
    }

    SUBCASE("struct array")
    {
    }
}

#endif
