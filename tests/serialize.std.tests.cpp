#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>

#include <pulmotor/serialize.hpp>
#include <pulmotor/std/utility.hpp>
#include <pulmotor/yaml_archive.hpp>
#include <pulmotor/binary_archive.hpp>

#include "common_types.hpp"
#include "common_archive.hpp"

#include <pulmotor/std/string.hpp>
#include <pulmotor/std/vector.hpp>

static pulmotor::romu3 r3;
TEST_CASE_TEMPLATE("string", ArchPair, YamlArchiveTest, BinaryArchiveTest)
{
    using namespace pulmotor;

    ArchPair ap;

    SUBCASE("empty")
    {
        std::string s, x("x");
        ap.o | s;

        auto i = ap.make_reader();
        i | x;
        CHECK(x.empty());
    }

    SUBCASE("empty multiple")
    {
        std::string s;
        size_t arr_size = 3;
        ap.o | array_size(arr_size) | s | s | s | end_array();

        std::string x, y("hog"), z(128, 'c');
        auto i = ap.make_reader();
        i | array_size(arr_size) | x | y | z | end_array();
        CHECK(x.empty());
        CHECK(y.empty());
        CHECK(z.empty());
    }

    // SUBCASE("content")
    // {
    //     std::string ss1("abcd");
    //     ap.o | ss1;

    //     std::string xs1;
    //     auto i = ap.make_reader();
    //     i | xs1;

    //     CHECK(xs1 == ss1);
    // }

    // SUBCASE("content multi")
    // {
    //     std::string ss("ab:cd");
    //     std::string sl("12: [ 34 ]567890qwertyuiopasdfghjklzxcvbnmQWERTYUIOPASDFGHJKLZXCVBNM");

    //     size_t arr_size = 2;
    //     ap.o | array_size(arr_size) | ss | sl * yaml_fmt(yaml::double_quoted) | end_array();

    //     std::string xs, xl;
    //     auto i = ap.make_reader();
    //     i | array_size(arr_size) | xs | xl | end_array();

    //     CHECK(xs == ss);
    //     CHECK(xl == sl);
    // }

    SUBCASE("various lengths")
    {
        constexpr size_t N=100;
        std::string sa[N];
        r3.reset();
        for (size_t i=0; i<N; ++i) {
            sa[i].reserve(i+1);
            for(size_t q=0; q<i; ++q) {
                char c = '0' + r3.range(128-'0');
                if (c == '\\' || c == '"' || c == '\'' ) c = '?';
                sa[i] += c;
            }
        }

        SUBCASE("serialize") {
            for (size_t i=1;i<N; ++i)
            {
                auto sf = [](auto& ar, std::string& s) { ar | s * yaml_fmt(yaml::fmt_flags::double_quoted); };

                ArchPair ap1;
                sf(ap1.o, sa[i]);

                std::string xs;
                auto ii = ap1.make_reader();
                sf(ii, xs);

                std::string const& s = sa[i];
                CHECK(s == xs);
            }
        }
    }
}

TEST_CASE_TEMPLATE("vector", ArchPair, YamlArchiveTest, BinaryArchiveTest)
{
    using namespace ptr_types;
    using namespace pulmotor;

    ArchPair ap;

    SUBCASE("empty ints")
    {
        std::vector<int> v, x(1, 5);
        ap.o | v;

        auto i = ap.make_reader();
        i | x;
        CHECK(x.empty());
    }

    SUBCASE("empty structs")
    {
        std::vector<A> v, x(1, A{7});
        ap.o | v;

        auto i = ap.make_reader();
        i | x;
        CHECK(x.empty());
    }

    SUBCASE("int")
    {
        std::vector<int> v{10, 20, 30, 40}, x;
        ap.o | v;

        auto i = ap.make_reader();
        i | x;

        CHECK(x.size() == v.size());
        CHECK(x == v);
    }

    // SUBCASE("int ptr")
    // {
    //     std::vector<int*> v{new int(10), new int(20)};
    //     ar | v;

    //     archive_vector_in i(ar.data);
    //     std::vector<int*> x;
    //     i | x;

    //     CHECK(x != v);
    //     CHECK(v.size() == x.size());
    //     CHECK(v.size() == x.size());
    //     for(auto z : v) delete z;
    //     for(auto z : x) delete z;
    // }

    // SUBCASE("struct")
    // {
    //     std::vector<X> v{X{10}, X{20}, X{30}, X{40}};
    //     ar | v;

    //     archive_vector_in i(ar.data);
    //     std::vector<X> x;
    //     i | x;

    //     CHECK(x.size() == v.size());
    //     CHECK(x == v);
    // }

    // SUBCASE("struct ctor")
    // {
    //     std::vector<A> v{A{10}, A{20}, A{30}, A{40}};
    //     ar | v;

    //     archive_vector_in i(ar.data);
    //     std::vector<A> x;
    //     i | x;

    //     CHECK(x.size() == v.size());
    //     CHECK(x == v);
    // }

    // SUBCASE("struct ptr ctor")
    // {
    //     std::vector<A*> v{new A{15}, new A{25}};
    //     ar | v;

    //     archive_vector_in i(ar.data);
    //     std::vector<A*> x;
    //     i | x;

    //     CHECK(x.size() == v.size());
    //     CHECK((std::equal(x.begin(), x.end(), v.begin(), v.end(), [](auto a, auto b) { return *a == *b; })) == true);
    //     for(auto z : v) delete z;
    //     for(auto z : x) delete z;
    // }
}

