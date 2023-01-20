#include <ryml.hpp>

#include <string>
using namespace std::string_literals;

// struct yaml_access
// {
//     size_t m_currid;

//     void start_child(ryml::Tree& r, size_t parent) { m_currid = r.first_child(parent); }

//     ryml::csubstr skey(ryml::Tree& r) {
//         if (ryml::NodeData const* n = r.get(m_currid)) {
//             if (n->is_keyval() && n->keysc()


//         }
//     }
// };

void pn(std::string const& prefix, ryml::Tree& y, size_t node) {

    ryml::NodeData* n = y.get(node);

    auto gg = [&y](size_t node, std::string const& p, auto probe, auto get) -> std::string
    {
        if ( (y.*probe)(node)) {
            ryml::csubstr const& s = (y.*get)(node);
            std::string_view sv = !s.empty() ? std::string_view(s.data(), s.size()) : "<null>";
            return p + std::string(sv.data(), sv.size());
        } else
            return "";
    };

    auto ss = [](ryml::csubstr const& s) {
        return s.empty() ? std::string() : std::string(s.data(), s.size());
    };


    printf("%s(type:%s)", prefix.c_str(), y.type_str(node));
    printf(" key:%d val:%d root:%d cont:%d keyval:%d map:%d seq:%d strm:%d val:%d doc:%d ktag:%d vtag:%d keyanch:%d valanch:%d\n",
        y.has_key(node), y.has_val(node), y.is_root(node), y.is_container(node), y.is_keyval(node), y.is_map(node), y.is_seq(node), y.is_stream(node), y.is_val(node), y.is_doc(node),
        y.has_key_tag(node), y.has_val_tag(node),
        y.has_key_anchor(node), y.has_val_anchor(node)
        );

    if (y.has_key(node))
        printf("%s# %s%s%s\n",
            prefix.c_str(),
            ss(y.key(node)).c_str(),
                gg(node, " tag:!", &ryml::Tree::has_key_tag, &ryml::Tree::key_tag).c_str(),
                gg(node, " anchor:*", &ryml::Tree::has_key_anchor, &ryml::Tree::key_anchor).c_str()
        );


    if (y.has_val(node))
        printf("%s--> %s\n", prefix.c_str(), ss(y.val(node)).c_str());
    else if (y.is_container(node)) {
        for (size_t i=y.first_child(node); i != ryml::NONE; i = y.next_sibling(i))
        {
            pn(prefix + "  ", y, i);
        }
    }
}

void dump_yaml(char const* yt)
{
    printf("yaml___\n%s\n", yt);
    ryml::Tree y = ryml::parse(ryml::to_csubstr(yt));
    pn("", y, y.root_id());
    // for(size_t i = y.first_child(y.root_id()); i != ryml::NONE; i = y.next_sibling(i))
    //     pn("  ", y, i);
    printf("\n");
}

void test_yaml()
{
    char yt[] =
R"(
a: 1
x:
arr:
    - 1
    - q: 10
      w: "ceramic"
    - 3
b: 3
name: hello
c: "50"
)";
//    dump_yaml(yt);
    char yt_empty[] = "";
    dump_yaml(yt_empty);

    char yt_1value[] = "256";
    dump_yaml(yt_1value);

    char yt_3value[] = R"(---
100
---
abc
---
a: 3
)";
    dump_yaml(yt_3value);

    char yt_arr1[] = R"([1, 2, 3])";
    dump_yaml(yt_arr1);

    char yt_arr2[] = R"([ [1, 2, 3] ])";
    dump_yaml(yt_arr2);

    char yt_am1[] = R"(
- a: 1
  b: 2
  c:
    x: F1
    y: F2
- 20
- 30
)";
    dump_yaml(yt_am1);

    char yt_m1[] = R"(a: 1)";
    dump_yaml(yt_m1);

    char yt_m2[] = R"(a: [1, 2])";
    dump_yaml(yt_m2);

    char yt_m3[] = R"(a: 1
b: 10
c: 100
)";
    dump_yaml(yt_m3);

    char yt_stream1[] = R"(---
a: [1, 2])
---
[4])";
    dump_yaml(yt_stream1);
}
