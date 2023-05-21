#include <pulmotor/serialize.hpp>
#include <iostream>

// #include "common_archive.hpp"

namespace test_types
{

struct A {
    int x;
    bool operator==(A const& a) const = default;
    template<class Ar> void serialize(Ar& ar, unsigned version) { ar | x; }
    friend std::ostream& operator<<(std::ostream& os, A const& a) { os << '{' << a.x << '}'; return os; }
};

struct Ak {
    int x;
    bool operator==(Ak const& a) const = default;
    template<class Ar> void serialize(Ar& ar, unsigned version) { ar | NV(x); }
    friend std::ostream& operator<<(std::ostream& os, Ak const& a) { os << '{' << a.x << '}'; return os; }
};


// same as as, but marked to be serialized with no prefix (below)
struct An {
    int x;
    bool operator==(An const& a) const = default;
    template<class Ar> void serialize(Ar& ar, unsigned version) { ar | x; }
    friend std::ostream& operator<<(std::ostream& os, An const& a) { os << '{' << a.x << '}'; return os; }
};

struct A2 {
    A a;
    int y = 2; // appeared in v2

    bool operator==(A2 const&) const = default;
    template<class Ar> void serialize(Ar& ar, unsigned version) {
        ar | a;
        if (version >= 2) ar | y;
    }
    friend std::ostream& operator<<(std::ostream& os, A2 const& a) { os << '{' << a.a << ", " << a.y << '}'; return os; }
};

struct C {
    int x[2];

    bool operator==(C const&) const = default;
    template<class Ar> void serialize(Ar& ar, unsigned version) { ar | x; }
};

struct P {
    A* a;
    int y;
    int* z;

    P(P const&) = delete;
    P& operator=(P const&) = delete;

    P(int xx, int yy, int zz) : a(new A{xx}), y(yy), z(new int(zz)) {}
    ~P() { delete a; delete z; }

    bool operator==(P const&) const = default;
    template<class Ar> void serialize(Ar& ar, unsigned version) { ar | a | y | z; }
};

struct N {
    A a;
    template<class Ar> void serialize(Ar& ar, unsigned version) { ar | a; }
};

} // test_types

namespace ptr_types
{
    struct A {
        int x;
        explicit A(int xx) : x(xx) {}

        template<class Ar> void save_construct(Ar& ar, unsigned version) {
            ar | x;
        }
        template<class Ar, class F> static void load_construct(Ar& ar, pulmotor::ctor<A, F> const& c, unsigned version) {
            int xx;
            ar | xx;
            c(xx);
        }
        bool operator==(A const&) const = default;
    };

    doctest::String toString(A const& a) { return ("{" + std::to_string(a.x) + "}").c_str(); }

    struct P {
        std::aligned_storage<sizeof(A)>::type a[1];

        explicit P(int aa) { new (a) A (aa); }
        ~P() { ((A*)a)->~A(); }

        template<class Ar> void serialize(Ar& ar, unsigned version) {
            if constexpr(Ar::is_reading)
                ((A*)a)->~A();
            ar | pulmotor::place<A>(a);
        }
        A& getA() { return *(A*)a; }
    };

    struct Za {
        A* px{nullptr};
        std::allocator<A> xa;

        using at = std::allocator_traits<decltype(xa)>;

        explicit Za(int aa) {
            px = xa.allocate(1);
            at::construct(xa, px, aa);
        }
        ~Za() {
            at::destroy(xa, px);
        }

        template<class Ar> void serialize(Ar& ar, unsigned version) {
            if constexpr(Ar::is_reading) at::destroy(xa, px);
            ar | pulmotor::alloc(px, [this]() { return at::allocate(xa, 1); } );
        }
    };

    struct Zb {
        A* px{nullptr};
        std::allocator<A> xa;

        using at = std::allocator_traits<decltype(xa)>;

        Zb() { }
        Zb(int aa) { px = xa.allocate(1); at::construct(xa, px, aa); }
        ~Zb() { at::destroy(xa, px); }

        template<class Ar> void serialize(Ar& ar, unsigned version) {
            ar | pulmotor::alloc(px, [this]() { return at::allocate(xa, 1); }, [this](A* p) { at::destroy(xa, p); } );
        }
    };

    struct X
    {
        int x;
        template<class Ar> void serialize(Ar& ar) { ar | x; }
        bool operator==(X const&) const = default;
    };
    doctest::String toString(X const& x) { return ("{" + std::to_string(x.x) + "}").c_str(); }

    struct Y
    {
        X* px {nullptr};

        ~Y() { delete px; }
        void init(int x) { px = new X{x}; }
        template<class Ar> void serialize(Ar& ar) { ar | px; }
    };
} // ptr_types
