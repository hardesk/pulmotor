#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>
#include <pulmotor/yaml_emitter.hpp>
#include <pulmotor/yaml_archive.hpp>
#include <string_view>
#include <pulmotor/serialize.hpp>

#define TEST_YAML_PARSE

#ifdef TEST_YAML_PARSE
#include <c4/std/string.hpp>
#include <ryml.hpp>
#endif

using namespace pulmotor;
//static romu3 r3;

struct X { int z, w; };
template<class Ar>
void serialize(Ar& ar, X& x) {
    ar | nv("z", x.z) | nv("w", x.w);
}

TEST_CASE("yaml writer")
{
    std::stringstream ss(std::ios_base::out|std::ios_base::binary);
    yaml::writer1 ws(ss);

    using namespace std::literals;

    SUBCASE("value")
    {
        SUBCASE("scalar")
        {
            ws.value("one"sv);
            CHECK(ss.str() == "one\n");
        }

        SUBCASE("block_folded")
        {
            ws.value("one"sv, yaml::fmt_flags::block_folded);
            CHECK(ss.str() == ">-\n one\n");
        }

        SUBCASE("block_literal")
        {
            ws.value("one"sv, yaml::fmt_flags::block_literal);
            CHECK(ss.str() == "|-\n one\n");
        }
    }

    SUBCASE("encoder")
    {
        using namespace pulmotor::yaml;
        SUBCASE("int")
        {
            encoder::format_value(ws, 10);
            CHECK(ss.str() == "10\n");
        }
        SUBCASE("string")
        {
            encoder::format_value(ws, "hello world");
            CHECK(ss.str() == "hello world\n");
        }
        SUBCASE("string sq")
        {
            encoder::format_value(ws, "hello world", fmt_flags::single_quoted|fmt_flags::is_text);
            CHECK(ss.str() == "'hello world'\n");
        }
        SUBCASE("string dq")
        {
            encoder::format_value(ws, "hello world", fmt_flags::flow|fmt_flags::double_quoted|fmt_flags::is_text);
            CHECK(ss.str() == "\"hello world\"\n");
        }
    }

    SUBCASE("array")
    {
        auto do_collection = [](yaml::writer1& w, yaml::fmt_flags seq_flags, yaml::fmt_flags v_flags = yaml::fmt_flags::fmt_none) {
            w.sequence(seq_flags);
            w.value("1"sv, v_flags);
            w.value("2"sv, v_flags);
            w.end_collection();
        };

        SUBCASE("array block")
        {
            do_collection(ws, yaml::fmt_flags::fmt_none);
            CHECK(ss.str() == "- 1\n- 2\n");
        }

        SUBCASE("array flow")
        {
            do_collection(ws, yaml::fmt_flags::flow);
            CHECK(ss.str() == "[ 1, 2 ]");
        }

        SUBCASE("array flow single quoted")
        {
            do_collection(ws, yaml::fmt_flags::flow, yaml::fmt_flags::single_quoted);
            CHECK(ss.str() == "[ '1', '2' ]");
        }

        SUBCASE("array flow duoble quoted")
        {
            do_collection(ws, yaml::fmt_flags::flow, yaml::fmt_flags::double_quoted);
            CHECK(ss.str() == "[ \"1\", \"2\" ]");
        }
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
            ws.end_collection();

        ws.end_collection();

        CHECK(ss.str() == "a: 1\nb: 2\nc: 3\nd:\n - &xx 100\n - 200\n - *xx\n");
    }

    SUBCASE("properties")
    {
        ws.mapping();
        ws.key("a"sv);
        ws.anchor("xx"sv);
        ws.tag("!!str"sv);
        ws.value("100"sv);
        ws.end_collection();
        CHECK(ss.str() == "a: &xx !!str 100\n");
    }

    SUBCASE("properties flow")
    {
        ws.mapping(yaml::flow);
        ws.anchor("xk"sv);
        ws.key("a"sv);
        ws.anchor("xv"sv);
        ws.tag("!!str"sv);
        ws.value("100"sv);

        ws.key("b"sv);
            ws.sequence(yaml::flow);
            ws.anchor("y1"sv);
            ws.value("200"sv);
            ws.value("*xv"sv);
            ws.end_collection();
        ws.end_collection();
        CHECK(ss.str() == "{ &xk a: &xv !!str 100, b: [ &y1 200, *xv ] }");
    }

    SUBCASE("array flow")
    {
        ws.sequence(yaml::flow);
        ws.value("1"sv);
        ws.value("2"sv);
            ws.sequence(yaml::flow);
            ws.value("10"sv);
            ws.value("20"sv);
            ws.end_collection();
        ws.end_collection();
        CHECK(ss.str() == "[ 1, 2, [ 10, 20 ] ]");
    }

    SUBCASE("mapping + mapping")
    {
        ws.mapping();
            ws.key("A");
            ws.mapping();
                ws.key_value("x"sv, "1"sv);
            ws.end_collection();
            ws.key_value("y"sv, "2"sv);
        ws.end_collection();

        CHECK(ss.str() == "A:\n x: 1\ny: 2\n");
    }

    SUBCASE("mapping + sequence")
    {
        ws.mapping();
            ws.key("x"sv);
            ws.sequence();
                ws.value("2"sv);
                ws.value("3"sv);
                ws.value("5"sv);
            ws.end_collection();
            ws.key_value("y"sv, "2"sv);
        ws.end_collection();

        CHECK(ss.str() == "x:\n - 2\n - 3\n - 5\ny: 2\n");
    }

    SUBCASE("sequence + mapping")
    {
        ws.sequence();
            ws.mapping();
                ws.key_value("a"sv, "1"sv);
                ws.key_value("b"sv, "2"sv);
            ws.end_collection();
            ws.value("X");
        ws.end_collection();
        CHECK(ss.str() == "-\n a: 1\n b: 2\n- X\n");
    }

    SUBCASE("sequence + sequence")
    {
        ws.sequence();
            ws.sequence();
                ws.value("2"sv);
                ws.value("4"sv);
            ws.end_collection();
            ws.value("x"sv);
        ws.end_collection();
        CHECK(ss.str() == "-\n - 2\n - 4\n- x\n");
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
            ws.end_collection();

        ws.key("e"sv);
            ws.mapping(yaml::flow);
            ws.key_value("x"sv, "5"sv);
            ws.key_value("y"sv, "10"sv);
            ws.end_collection();

        ws.end_collection();
        CHECK(ss.str() == "{ a: 10, b: 20, c: 30, d: [ &xx 1, &yy 2 ], e: { x: 5, y: 10 } }");
    }

    auto ryroot = [](std::string const& yaml) -> std::string {
        ryml::Tree y = ryml::parse_in_arena(ryml::to_csubstr(yaml));
        std::string v (y.rootref().val().str, y.rootref().val().len);
        return v;
    };

    SUBCASE("block")
    {
        SUBCASE("literal plain") {
            ws.value("hello", yaml::block_literal|yaml::is_text);
            CHECK(ss.str() == "|-\n hello\n");
#ifdef TEST_YAML_PARSE
            CHECK(ryroot(ss.str()) == "hello");
#endif
        }

        SUBCASE("literal eol") {
            ws.value("hello\nworld\n", yaml::block_literal|yaml::is_text);
            CHECK(ss.str() == "|\n hello\n world\n");
#ifdef TEST_YAML_PARSE
            CHECK(ryroot(ss.str()) == "hello\nworld\n");
#endif
        }

        SUBCASE("fold") {
            ws.value("hello\nworld", yaml::block_folded|yaml::is_text);
            CHECK(ss.str() == ">-\n hello\n world\n");
#ifdef TEST_YAML_PARSE
            CHECK(ryroot(ss.str()) == "hello world");
#endif
        }

        SUBCASE("clip") {
            ws.value("hello\nworld\n", yaml::block_literal|yaml::eol_clip|yaml::is_text);
            CHECK(ss.str() == "|\n hello\n world\n");
#ifdef TEST_YAML_PARSE
            CHECK(ryroot(ss.str()) == "hello\nworld\n");
#endif
        }
        SUBCASE("strip") {
            ws.value("hello\nworld\n", yaml::block_folded|yaml::eol_strip|yaml::is_text);
            CHECK(ss.str() == ">-\n hello\n world\n");
#ifdef TEST_YAML_PARSE
            CHECK(ryroot(ss.str()) == "hello world");
#endif
        }
        SUBCASE("keep") {
            ws.value("hello\nworld\n\n", yaml::block_folded|yaml::eol_keep|yaml::is_text);
            CHECK(ss.str() == ">+\n hello\n world\n \n");
#ifdef TEST_YAML_PARSE
            CHECK(ryroot(ss.str()) == "hello world\n\n");
#endif
        }
        SUBCASE("keep indented") {
            ws.value("hello\n world\n\n", yaml::block_literal|yaml::eol_keep|yaml::is_text);
            CHECK(ss.str() == "|+\n hello\n  world\n \n");
#ifdef TEST_YAML_PARSE
            CHECK(ryroot(ss.str()) == "hello\n world\n\n");
#endif
        }
//         SUBCASE("indent") {
//             ws.value("hello\nworld\n all", yaml::block_literal|yaml::writer::indent(3)|yaml::is_text);
//             CHECK(ss.str() == "|-3\n   hello\n   world\n    all\n");
// #ifdef TEST_YAML_PARSE
//             CHECK(ryroot(ss.str()) == "hello\nworld\n all");
// #endif
//         }
    }
}


TEST_CASE("yaml archive write")
{
    std::stringstream ss;
    yaml::writer1 ws(ss);
    archive_write_yaml o0(ws, archive_yaml::no_stream);
    archive_write_yaml oK(ws, archive_yaml::no_stream|archive_yaml::keyed);

    SUBCASE("basic") {
        SUBCASE("integer") {
            int a = 1;
            o0 | a;
            CHECK(ss.str() == "1\n");
        }
        SUBCASE("string") {
            std::string a = "hello world";
            o0 | a;
            CHECK(ss.str() == "hello world\n");
        }
        SUBCASE("string qouted single") {
            std::string a = "hello world";
            o0 | a * yaml_fmt(yaml::single_quoted);
            CHECK(ss.str() == "'hello world'\n");
        }
        SUBCASE("string qouted double") {
            std::string a = "hello world";
            o0 | a * yaml_fmt(yaml::double_quoted);
            CHECK(ss.str() == "\"hello world\"\n");
        }
    }

    SUBCASE("array") {
        int ai[2] = { 100, 200 };
        std::string as[2] = { "hello", "world" };
        SUBCASE("ints") {
            o0 | pulmotor::array(ai, 2);
            CHECK(ss.str() == "- 100\n- 200\n");
        }
        SUBCASE("strings") {
            o0 | pulmotor::array(as, 2);
            CHECK(ss.str() == "- hello\n- world\n");
        }
        SUBCASE("strings split") {
            size_t s = 2;
            o0 | pulmotor::array_size(s);
            o0 | array_data(as, 2);
            CHECK(ss.str() == "- hello\n- world\n");
        }
        SUBCASE("strings more") {
            std::string b = "and more";
            size_t s = 2;
            o0 | pulmotor::array_size(s);
            o0 | as[0] | as[1] | b * yaml_fmt(yaml::fmt_flags::single_quoted) | end_array();
            CHECK(ss.str() == "- hello\n- world\n- 'and more'\n");
        }
    }
    SUBCASE("struct") {
        SUBCASE("single") {
            X x{100, 200};
            oK | x;
            CHECK(ss.str() == "z: 100\nw: 200\n");
        }
        SUBCASE("array") {
            X x[2] = { {101, 201}, {102, 202}};
            oK | pulmotor::array(x, 2);
            CHECK(ss.str() == "-\n z: 101\n w: 201\n-\n z: 102\n w: 202\n");
        }
        SUBCASE("array native") {
            X x[2] = { {101, 201}, {102, 202}};
            oK | x;
            CHECK(ss.str() == "-\n z: 101\n w: 201\n-\n z: 102\n w: 202\n");
        }
    }

    SUBCASE("nv") {
        int a = 1;
        oK.begin_object();
        oK | nv("a", a) | nv("c", a);
        oK.end_object();

        CHECK(ss.str() == "a: 1\nc: 1\n");
    }

    SUBCASE("nv struct") {
        X x{100, 200};
        oK.begin_object();
        oK | nv("x", x);
        oK.end_object();
        CHECK(ss.str() == "x:\n  z: 100\n  w: 200\n");
    }
}


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
