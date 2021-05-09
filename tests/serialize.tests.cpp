#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>
#include <pulmotor/serialize.hpp>

using namespace pulmotor;

template<class T>
struct basic_tester {
	void operator()(T value) {
		std::stringstream ss;
		pulmotor::sink_ostream so(ss);
		pulmotor::sink_archive oar(so);

		// write
		oar | value;

		char buffer[sizeof(value)];
		memcpy(buffer, ss.str().data(), ss.str().size());
		CHECK(value == *reinterpret_cast<decltype(value)*>(buffer));

		// read back and compare
		pulmotor::source_buffer in(buffer, sizeof buffer);
		pulmotor::archive_whole iar(in);
		T readValue = 0;
		iar | readValue;
		CHECK( readValue == value );
	}
};

namespace test_types
{
	struct A {
		int x;

		bool operator==(A const& a) const = default;
		template<class Ar> void serialize(Ar& ar, unsigned version) { ar | x; }
	};

	struct B {
		A a;
		int y;

		bool operator==(B const&) const = default;
		template<class Ar> void serialize(Ar& ar, unsigned version) { ar | a | y; }
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

}

namespace detect_serialize
{

struct X1 { }; template<class Ar> void serialize(Ar& ar, X1& x) { }
struct X2 { template<class Ar> void serialize(Ar& ar) { } };
struct X3 { }; template<class Ar> void serialize(Ar& ar, X3& x, unsigned version) { }
struct X4 { template<class Ar> void serialize(Ar& ar, unsigned version) { } };

struct X5 { }; template<class Ar> void load_construct(Ar& ar, X5*& o, unsigned) { }
struct X6 { template<class Ar> static void load_construct(Ar& ar, X6*&, unsigned version) { } };
struct X7 { }; template<class Ar> void save_construct(Ar& ar, X7& x, unsigned version) { }
struct X8 { template<class Ar> void save_construct(Ar& ar, unsigned version) { } };

enum { S = 1, SM = 2, SV = 4, SMV = 8, LC = 16, LCM = 32, SC = 64, SCM = 128 };
template<class T, unsigned CHECKS, unsigned Sn, unsigned LCn, unsigned SCn>
struct test_type
{
	static void check();
};

template<class T, unsigned CHECKS, unsigned Sn, unsigned LCn, unsigned SCn>
void test_type<T,CHECKS,Sn,LCn,SCn>::check()
{
	using Ar = pulmotor::pulmotor_archive;

	CHECK(access::detect<Ar>::has_serialize<T>::value == ((CHECKS & S) != 0));
	CHECK(access::detect<Ar>::has_serialize_mem<T>::value == ((CHECKS & SM) != 0));
	CHECK(access::detect<Ar>::has_serialize_version<T>::value == ((CHECKS & SV) != 0));
	CHECK(access::detect<Ar>::has_serialize_mem_version<T>::value == ((CHECKS & SMV) != 0));
	CHECK(access::detect<Ar>::has_load_construct<T>::value == ((CHECKS & LC) != 0));
	CHECK(access::detect<Ar>::has_load_construct_mem<T>::value == ((CHECKS & LCM) != 0));
	CHECK(access::detect<Ar>::has_save_construct<T>::value == ((CHECKS & SC) != 0));
	CHECK(access::detect<Ar>::has_save_construct_mem<T>::value == ((CHECKS & SCM) != 0));

	CHECK(access::detect<Ar>::count_serialize<T>::value == Sn);
	CHECK(access::detect<Ar>::count_load_construct<T>::value == LCn);
	CHECK(access::detect<Ar>::count_save_construct<T>::value == SCn);
}
}

TEST_CASE("detect")
{
	using namespace detect_serialize;


	test_type<X1, S  , 1, 0, 0>::check();
	test_type<X2, SM , 1, 0, 0>::check();
	test_type<X3, SV , 1, 0, 0>::check();
	test_type<X4, SMV, 1, 0, 0>::check();
	
//	test_type<X5, LC , 0, 1, 0>::check();
	test_type<X6, LCM, 0, 1, 0>::check();
	test_type<X7, SC , 0, 0, 1>::check();
	test_type<X8, SCM, 0, 0, 1>::check();
}

TEST_CASE("value serialize")
{
	using namespace pulmotor;

	SUBCASE("arithmetic")
	{
		basic_tester<bool>()(false);
		basic_tester<bool>()(true);
		basic_tester<char>()('a');
		basic_tester<signed char>()(-1);
		basic_tester<unsigned char>()(250);
		basic_tester<short>()(16384);
		basic_tester<short>()(-1);
		basic_tester<unsigned short>()(65535);
		basic_tester<int>()(100000);
		basic_tester<int>()(-100000);
		basic_tester<unsigned>()(3002001000);
		basic_tester<long>()(3002001000);
		basic_tester<unsigned long>()(3002001000);
		basic_tester<long long>()(3002001000);
		basic_tester<unsigned long long>()(3002001000);

		basic_tester<u8>()(0xff);
		basic_tester<s8>()(0x7f);
		basic_tester<s8>()(-0x80);
		basic_tester<u16>()(0xffff);
		basic_tester<s16>()(0x7fff);
		basic_tester<s16>()(-0x8000);
		basic_tester<u32>()(0xffff'ffffU);
		basic_tester<s32>()(0x7fff'ffff);
		basic_tester<s32>()(-0x8000'0000);
		basic_tester<u64>()(0xffff'ffff'ffff'ffff);
		basic_tester<s64>()(0x7fff'ffff'ffff'ffff);
		basic_tester<s64>()(-0x8000'0000'0000'0000);
	}

	using namespace test_types;
	archive_vector_out ar;
	SUBCASE("struct")
	{
		A a{1234};
		ar | a;

		archive_vector_in i(ar.data);
		A x;
		i | x;

		CHECK( a == x );
	}

	SUBCASE("primitive array")
	{
		int a[2] = {1, 120};
		ar | a;

		archive_vector_in i(ar.data);
		int x[2];
		i | x;
		CHECK( a[0] == x[0] );
		CHECK( a[1] == x[1] );
	}

	SUBCASE("array")
	{
		A a[2] = {1234, 5678};
		ar | a;

		archive_vector_in i(ar.data);
		A x[2];
		i | x;
		CHECK( a[0] == x[0] );
		CHECK( a[1] == x[1] );
	}
	
	SUBCASE("nested struct")
	{
		N n { {1233} };
		ar | n;

		B b{1111, 2222};
		ar | b;


		archive_vector_in i(ar.data);
		N n1;
		i | n1;
		CHECK( n1.a.x == n.a.x );

		B x;
		i | x;
		CHECK( b.a == x.a );
		CHECK( b.y == x.y );

	}
}

namespace ptr_types
{
	struct A {
		int x;
		explicit A(int xx) : x(xx) {}

		template<class Ar> void save_construct(Ar& ar, unsigned version) {
			ar | x;
		}
		template<class Ar> static void load_construct(Ar& ar, A* x, unsigned version) {
			int xx;
			ar | xx;
			new (x) A (xx);
		}
	};
	
	struct P {
		std::aligned_storage<sizeof(A)>::type a[1];

		explicit P(int aa) { new (a) A (aa); }
		~P() { ((A*)a)->~A(); }

		template<class Ar> void serialize(Ar& ar, unsigned version) {
			if constexpr(Ar::is_reading)
				((A*)a)->~A();
			ar | place<A>(a);
		}
		A& getA() { return *(A*)a; }
	};

	/*struct Za {
		X* px;
		std::allocator<X> xa;

 		using at = std::allocator_traits<decltype(xa)>;

		explicit Za(int xx) {
			px = xa.allocate(1);
			at::construct(xa, px, xx);
		}
		~Za() {
			at::destroy(xa, px);
		}

		template<class Ar> void serialize(Ar& ar, unsigned version) {
			ar | alloc(px, [this]() { return at::allocate(xa, 1); } );
			//ar | px * alloc([](&this) { return at::allocate(xa, 1); } );
		}
	};*/
}

TEST_CASE("ptr serialize")
{
	using namespace ptr_types;
	archive_vector_out ar;

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

	SUBCASE("placement new")
	{
		std::aligned_storage<sizeof(A)> a[1];
		new (a) A(123456);
		ar | place<A>(a);

		P p1(100);
		ar | p1;

		std::aligned_storage<sizeof(A)> aa[1];
		archive_vector_in i(ar.data);
		i | place<A>(aa);

		A& a1 = *reinterpret_cast<A*>(a);
		A& a2 = *reinterpret_cast<A*>(aa);

		CHECK(a1.x == a2.x);


		P p2(200);
		i | p2;
		CHECK(p1.getA().x == p2.getA().x);

	}

	SUBCASE("with allocator")
	{
	}

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
}
