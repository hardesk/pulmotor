#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>
#include <pulmotor/yaml_emitter.hpp>
#include <pulmotor/yaml_archive.hpp>
#include <string_view>
#include <pulmotor/serialize.hpp>

//#define TEST_YAML_PARSE

#ifdef TEST_YAML_PARSE
#include <c4/std/string.hpp>
#include <ryml.hpp>
#endif

using namespace pulmotor;
//static romu3 r3;

struct X { int z; };
template<class Ar>
void serialize(Ar& ar, X& x) {
    ar | nv("z", x.z);
}

TEST_CASE("yaml archive write")
{
    std::stringstream ss;
    yaml::writer_ostream ws(ss);

    archive_write_yaml o(ws, archive_yaml::no_stream);

    SUBCASE("basic") {
        int a = 1;
        o.begin_array();
        o | a;
        o.end_array();
        CHECK(ss.str() == "1\n");
    }

    SUBCASE("nv") {
        int a = 1;
        o.begin_object();
        o | nv("a", a) | nv("c", a);
        o.end_object();

        CHECK(ss.str() == "a: 1\n");
    }

    SUBCASE("nv struct") {
        X x{100};
        int a = 7;
        o.begin_object();
        o | nv("x", x) | nv("a", a);
        o.end_object();

        CHECK(ss.str() == "x:\n  z: 100\na: 1\n");
    }

}

TEST_CASE("yaml writer")
{
    std::stringstream ss(std::ios_base::out|std::ios_base::binary);
    yaml::writer_ostream ws(ss);

    using namespace std::literals;

    SUBCASE("value")
    {
        ws.value("one"sv);
        CHECK(ss.str() == "one\n");
    }

    SUBCASE("array")
    {
        ws.sequence();
        ws.value("1"sv);
        ws.value("2"sv);
        ws.end_container();
        CHECK(ss.str() == "- 1\n- 2\n");
    }

    SUBCASE("properties")
    {
        ws.mapping();
        ws.key("a"sv);
        ws.anchor("xx"sv);
        ws.tag("!str"sv);
        ws.value("100"sv);
        ws.end_container();
        CHECK(ss.str() == "a: &xx !!str 100\n");
    }

    SUBCASE("properties flow")
    {
        ws.mapping(yaml::flow);
        ws.anchor("x1"sv);
        ws.key("a"sv);
        ws.anchor("x2"sv);
        ws.tag("!str"sv);
        ws.value("100"sv);
        ws.key("b"sv);
            ws.sequence(yaml::flow);
            ws.anchor("y1"sv);
            ws.value("200"sv);
            ws.value("*x2"sv);
            ws.end_container();
        ws.end_container();
        CHECK(ss.str() == "{&x1 a: &x2 !!str 100, b: [&y1 200, *x2]}\n");
    }

    SUBCASE("array flow")
    {
        ws.sequence(yaml::flow);
        ws.value("1"sv);
        ws.value("2"sv);
            ws.sequence(yaml::flow);
            ws.value("10"sv);
            ws.value("20"sv);
            ws.end_container();
        ws.end_container();
        CHECK(ss.str() == "[1, 2, [10, 20]]\n");
    }

    SUBCASE("mapping")
    {
        ws.mapping();

        ws.key("a"sv);
        ws.value("1"sv);

        ws.key_value("b"sv, "2"sv);

        ws.key("c"sv);
        ws.value("3"sv);

        ws.key("d"sv);
            ws.sequence();
            ws.anchor("xx"sv);
            ws.value("100"sv);
            ws.value("200"sv);
            ws.value("*xx"sv);
            ws.end_container();

        ws.end_container();

        CHECK(ss.str() == "a: 1\nb: 2\nc: 3\nd:\n - &xx 100\n - 200\n - *xx\n");
    }

    SUBCASE("mapping flow")
    {
        ws.mapping(yaml::flow);
        ws.key("a"sv);
        ws.value("10"sv);
        ws.key_value("b"sv, "20"sv);
        ws.key("c"sv);
        ws.value("30"sv);

        ws.key("d"sv);
            ws.sequence(yaml::flow);
            ws.anchor("xx"sv);
            ws.value("1"sv);
            ws.anchor("yy"sv);
            ws.value("2"sv);
            ws.end_container();

        ws.key("e"sv);
            ws.mapping(yaml::flow);
            ws.key_value("x"sv, "5"sv);
            ws.key_value("y"sv, "10"sv);
            ws.end_container();

        ws.end_container();
        CHECK(ss.str() == "{a: 10, b: 20, c: 30, d: [&xx 1, &yy 2], e: {x: 5, y: 10}}\n");
    }

    SUBCASE("block")
    {
        SUBCASE("literal plain") {
            ws.value("hello", yaml::block_literal);
            CHECK(ss.str() == "|-\n hello\n");
#ifdef TEST_YAML_PARSE
            ryml::Tree y = ryml::parse(ryml::to_csubstr(ss.str()));
            CHECK(y.rootref().val() == "hello\nworld\n");
#endif
        }

        SUBCASE("literal eol") {
            ws.value("hello\nworld\n", yaml::block_literal);
            CHECK(ss.str() == "|\n hello\n world\n");
#ifdef TEST_YAML_PARSE
            ryml::Tree y = ryml::parse(ryml::to_csubstr(ss.str()));
            CHECK(y.rootref().val() == "hello\nworld\n");
#endif
        }

        SUBCASE("fold") {
            ws.value("hello\nworld", yaml::block_fold);
            CHECK(ss.str() == ">-\n hello\n world\n");
#ifdef TEST_YAML_PARSE
            ryml::Tree y = ryml::parse(ryml::to_csubstr(ss.str()));
            CHECK(y.rootref().val() == "hello world");
#endif
        }

        SUBCASE("clip") {
            ws.value("hello\nworld\n", yaml::block_literal|yaml::eol_clip);
            CHECK(ss.str() == "|\n hello\n world\n");
#ifdef TEST_YAML_PARSE
            ryml::Tree y = ryml::parse(ryml::to_csubstr(ss.str()));
            CHECK(y.rootref().val() == "hello\nworld\n");
#endif
        }
        SUBCASE("strip") {
            ws.value("hello\nworld\n", yaml::block_fold|yaml::eol_strip);
            CHECK(ss.str() == ">-\n hello\n world\n");
#ifdef TEST_YAML_PARSE
            ryml::Tree y = ryml::parse(ryml::to_csubstr(ss.str()));
            CHECK(y.rootref().val() == "hello world");
#endif
        }
        SUBCASE("keep") {
            ws.value("hello\nworld\n\n", yaml::block_fold|yaml::eol_keep);
            CHECK(ss.str() == ">+\n hello\n world\n \n");
#ifdef TEST_YAML_PARSE
            ryml::Tree y = ryml::parse(ryml::to_csubstr(ss.str()));
            std::string v (y.rootref().val().str, y.rootref().val().len);
            CHECK(v == "hello world\n\n");
#endif
        }
        SUBCASE("keep indented") {
            ws.value("hello\n world\n\n", yaml::block_literal|yaml::eol_keep);
            CHECK(ss.str() == "|+\n hello\n  world\n \n");
#ifdef TEST_YAML_PARSE
            ryml::Tree y = ryml::parse(ryml::to_csubstr(ss.str()));
            std::string v (y.rootref().val().str, y.rootref().val().len);
            CHECK(v == "hello\n world\n\n");
#endif
        }
        SUBCASE("indent") {
            ws.value("hello\nworld\n all", yaml::block_literal|yaml::writer::indent(3));
            CHECK(ss.str() == "|-3\n   hello\n   world\n    all\n");
#ifdef TEST_YAML_PARSE
            ryml::Tree y = ryml::parse(ryml::to_csubstr(ss.str()));
            std::string v (y.rootref().val().str, y.rootref().val().len);
            CHECK(v == "hello\nworld\n all");
#endif
        }
    }
}

#if 0
#include <pulmotor/yaml_archive.hpp>

TEST_CASE("yaml archive")
{
    sink_sstream ss;
    archive_yaml_writer ar(ss);
    archive_yaml_writer::context
        def {},
        flow { yaml::flow, 0, 0 },
        base64 { 0, true, 0 }
        ;

    SUBCASE("int") {
        ar.write_single(10, def);
        CHECK( ss.ss().str() == "10\n");
    }
    SUBCASE("int dash") {
        ar.set_flags(archive_yaml_writer::dash_first_stream);
        ar.write_single(10, def);
        CHECK( ss.ss().str() == "---\n10\n");
    }
    SUBCASE("int 2") {
        ar.write_single(10, def);
        ar.write_single(20, def);
        CHECK( ss.ss().str() == "10\n---\n20\n");
    }

    SUBCASE("array") {
        ar.begin_array(def);
        ar.write_single(10, def);
        ar.write_single(20, def);
        ar.end_container();
        CHECK( ss.ss().str() == "- 10\n- 20\n");
    }
    SUBCASE("array flow") {
        ar.begin_array(flow);
        ar.write_single(10, def);
        ar.write_single(20, def);
        ar.end_container();
        CHECK( ss.ss().str() == "[10, 20]\n");
    }

    SUBCASE("implicit mapping int 1") {
        ar.write_key("xx"sv, def);
        ar.write_single(10, def);
        CHECK( ss.ss().str() == "xx: 10\n");
    }

    SUBCASE("implicit mapping int 2") {
        ar.write_key("xx"sv, def);
        ar.write_single(10, def);
        ar.write_key("yy"sv, def);
        ar.write_single(20, def);
        CHECK( ss.ss().str() == "xx: 10\n---\nyy: 20\n");
    }

    SUBCASE("mapping") {
        ar.begin_object(def);
        ar.write_key("x", def);
        ar.write_single(10, def);
        ar.write_key("y", def);
        ar.write_single(20, def);
        ar.end_container();
        CHECK( ss.ss().str() == "x: 10\ny: 20\n");
    }

    SUBCASE("b64 value") {
        ar.encode_array(base64, "1234", 4);
        CHECK( ss.ss().str() == "!!binary MTIzNA==\n");
    }

}
#endif

/*
#include <pulmotor/yaml_archive.hpp>>
TEST_CASE("yaml archive")
{
    pulmotor::yaml_document ydoc("archive.yaml");
    pulmotor::archive_yaml_out ar (ydoc);

    SUBCASE("empty")
    {
    }

    SUBCASE("int")
    {
        ar.write_single(10);
        CHECK(ydoc.to_string() == "10\n");
    }

    SUBCASE("struct")
    {
        ar.begin_object(nullptr, 0);
        ar.write_single("x", 1, 100);
        ar.write_single("y", 1, 200);
        ar.end_object();
        CHECK(ydoc.to_string() ==
R"(x: 100
  y: 200
)");
    }

    SUBCASE("float")
    {
        float aa[5] = { 0.0f, 1.0f, 3.14159265f, 0.2f, 1.00000011921f };
        ar.write_data_array(aa, std::size(aa));
        CHECK(ydoc.to_string() == "as");
    }

    // void write_data_array (char const* name, size_t name_len, T const* arr, size_t size) {
    // void write_data_blob (char const* name, size_t name_len, T const* arr, size_t size) {
}*/
