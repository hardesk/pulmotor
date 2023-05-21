# pulmotor
Yet another implementation of a serialization library for C++ (retuires C++17). Mostly this library is an excersize in C++ coding to learn C++17 approaches to type magic.

It actually started as an exersize to make a "blit" serialization where objects are written out in such a way that the resulting block can be loaded whole and (after applying relocations) the memory can be used straight as a runtime data structure. That did work for simple cases, but for more complex structures it seemed to get out of hand pretty quickly and I didn't have the determination to figure out the approaches there. So in order to have something usable in addition to that I also wrote a serializer/deserializer. The latter needs to interpret stream when loading but is way more portable and well, usable. I may come back to `blit` approach later, but only if I find a strong need for it.

The serialization API is similar to boost serialization. Instead though, the library hijacks `|` operator. It has some features which are the main drive for this library:

- (existed in previous version, but with current facilities I scrapped this feature for now; `construct` may be sufficient). You can pass a context to children being serialized.
- supports placement new and std::allocator memory allocations for objects when serializing through pointer (again not sure how useful that'll prove to be actually).
- `emplace` support  while de-serializing objects eg. into a vector.
- variable length integers (I'm thinking for size data).

An example follows:

```
#include <pulmotor/pulmotor.hpp>

struct X
{
    int x;
    template<class Ar> void serialize(Ar& ar) { ar | x; }
};

struct Y
{
    X* px {nullptr};
    ~Y() { delete px; }

    void init(int a) { pa = new A(a); }

    template<class Ar> void serialize(Ar& ar, unsigned version) { ar | px; }
};

int main()
{
    pulmotor::archive_vector_out ar;

    Y y1, y2;
    y2.init(200);
    ar | y1 | y2;

    ar  | NV("y1", y1)
        | NV("y2", y1);

    using namespace std;
    std::fstream fs("data", ios::out|ios::binary|ios::trunc);
    fs.write(ar.data.data(), ar.data.size());

    Y z1, z2;
    pulmotor::archive_vector_in in(ar.data);
    in | z1 | z2;

    assert(z1.px == nullptr);
    assert(z2.px->x == y2.px->y);
}
```

## Serialization Structure

```
<serializable>      := <primitive>
                     | <primitive-array>
                     | VF <struct>
                     | VF <struct-array>
                     | VF <pointer>
                     | VF <pointer-array>
                     | PLACE(area) <pointer>
                     | CONSTRUCT(ptr) <pointer>
                     | ALLOC <pointer>
                     | PTR <pointer>
                     | BASE <struct>
                     | ARRAY-REF primitive-array | ARRAY-REF struct-array
                     | VU <size_t>
                     | NV(name) <serializable>

<pointer>           := PTR-ID? <serializable>
<pointer-array>     := SIZE (PTR-ID? <serializable>){SIZE}
<struct-array>      := SIZE <struct>{SIZE}
<struct>            := [--> call custom function] <serializable> *
<primitive-array>   := SIZE <primitive>{SIZE}
<primitive>         := arithmetic# | enum#
VF                  := FLAGS-and-version
```

## Building

I'm testing out build2 as a project building tool. Getting into it and setting up the project is not trivial.

```
b create: ../clang_build/,cxx config.cxx=clang++
b configure: ../clang_build/ config.cc.coptions="-g" config.cc.poptions="DEBUG" # configure for debug build
# b configure: ../clang_build/ config.cc.coptions="-O2" config.cc.poptions="NDEBUG" # configure for build release build
cd ../clang_build
b
```


## Testing

yaml structure https://nimyaml.org/testing.html

