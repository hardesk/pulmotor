#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>
#include <pulmotor/serialize.hpp>
#include <pulmotor/std/utility.hpp>

std::vector<char> operator"" _v(char const* s, size_t l) { return std::vector<char>(s, s + l); }

namespace std {

template<class T>
std::string toStringValue(T const& a)
{
	std::string s;
	if constexpr(std::is_same<T, std::string>::value) {
		return a;
	} else if constexpr(std::is_same<T, char>::value) {
		s += '\'';
		if (a < 32) s += '\\' + std::to_string((int)a); else s += a;
		s += '\'';
	} else if constexpr(std::is_pointer<T>::value && std::is_arithmetic<typename std::remove_pointer<T>::type>::value) {
		s += '&';
		s += std::to_string(*a);
	} else if constexpr(std::is_arithmetic<T>::value) {
		s += std::to_string(a);
	} else {
		if constexpr(std::is_pointer<T>::value) {
			s += '&';
			s += toString(*a).c_str();
		} else
			s += toString(a).c_str();
		//s += '?';
	}
	return s;
}

template<class T, class A>
doctest::String toString(std::vector<T, A> const& v) {
	std::string s;
	s.reserve(v.size() * sizeof(T)*7/2 + 5);
	for(size_t i=0; i<v.size(); ++i) {
		if (v.size() > 32 && (i>=24 && i<v.size()-4)) {
			if (i == 24)
				s += ", ...";
		} else {
			s += toStringValue(v[i]);
			if (i != v.size() - 1)
				s += ", ";
		}
	}
	return s.c_str();
}

template<class K, class T, class C, class A>
doctest::String toString(std::map<K, T, C, A> const& m) {
	std::string s;
	s.reserve(m.size() * sizeof(T)*7/2 * sizeof(K)*7/2 + 5);
	size_t i = 0;
	for(auto it = m.begin(); it != m.end(); ++it, ++i) {
		if (m.size() > 32 && (i>=24 && i<m.size()-4)) {
			if (i == 24)
				s += ", ...";
		} else {
			s += '(' + toStringValue(it->first) + ':' + toStringValue(it->second) + ')';
			if (i != m.size() - 1)
				s += ", ";
		}
	}
	return s.c_str();
}

} // std

template<class T>
struct basic_tester {
	void operator()(T value) {
		std::stringstream ss;
		pulmotor::sink_ostream so(ss);
		pulmotor::archive_sink oar(so);

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
	using F = decltype(f);
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


TEST_CASE("type util")
{
	CHECK( (std::is_same<pulmotor::arg_i<0, int, float, char>::type, int>::value) == true);
	CHECK( (std::is_same<pulmotor::arg_i<1, int, float, char>::type, float>::value) == true);
	CHECK( (std::is_same<pulmotor::arg_i<2, int, float, char>::type, char>::value) == true);
}

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

	SUBCASE("alignment")
	{
		using namespace std::string_literals;

		u8 a8{0x38};
		u16 a16{0x3631};
		u32 a32{0x32333233};
		u64 a64{0x3436343634363436};

		u8 x8;
		u16 x16;
		u32 x32;
		u64 x64;

		archive_vector_out ar;

		SUBCASE("8") {
			ar | a8 | a16 | a32 | a64;
			printf("ss: %s\n", ar.str().data());
			CHECK(ar.data == "8\0" "16323264646464"_v);

			archive_vector_in i(ar.data);
			i | x8 | x16 | x32 | x64;
			CHECK(a8==x8);
			CHECK(a16==x16);
			CHECK(a32==x32);
			CHECK(a64==x64);
		}

		SUBCASE("16") {
			ar | a16 | a32 | a64;
			CHECK(ar.data == "16\0\0" "323264646464"_v);
			archive_vector_in i(ar.data);
			i | x16 | x32 | x64;
			CHECK(a16==x16);
			CHECK(a32==x32);
			CHECK(a64==x64);
		}

		SUBCASE("32") {
			ar | a32 | a64;
			CHECK(ar.data == "3232\0\0\0\0""64646464"_v);
			archive_vector_in i(ar.data);
			i | x32 | x64;
			CHECK(a32==x32);
			CHECK(a64==x64);
		}

		SUBCASE("16\08") {
			ar | a16 | a8 | a32 | a64;
			CHECK(ar.data == "168\0""323264646464"_v);
			archive_vector_in i(ar.data);
			i | x16 | x8 | x32 | x64;
			CHECK(a16==x16);
			CHECK(a8==x8);
			CHECK(a32==x32);
			CHECK(a64==x64);
		}

		SUBCASE("16\032\08") {
			ar | a16 | a32 | a8 | a64;
			CHECK(ar.data == "16\0\0""32328\0\0\0\0\0\0\0""64646464"_v);
			archive_vector_in i(ar.data);
			i | x16 | x32 | x8 | x64;
			CHECK(a16==x16);
			CHECK(a32==x32);
			CHECK(a8==x8);
			CHECK(a64==x64);
		}
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

	SUBCASE("array of array")
	{
		A a[2][3] = { {1234, 5678, 9112}, {1234, 5678, 9556} };
		ar | a;

		archive_vector_in i(ar.data);
		A x[2][3];
		i | x;
		CHECK( a[0][0] == x[0][0] );
		CHECK( a[0][1] == x[0][1] );
		CHECK( a[0][2] == x[0][2] );
		CHECK( a[1][0] == x[1][0] );
		CHECK( a[1][1] == x[1][1] );
		CHECK( a[1][2] == x[1][2] );
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

	SUBCASE("base")
	{
		auto s = [&ar](auto& a)
		{
			ar | a;

			archive_vector_in i(ar.data);
			std::remove_reference_t<decltype(a)> x;
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
			// a=32768; while [[ $a -ne 0 ]]; do if [[ $a -gt 127 ]]; then echo $((a & 0x7f|0x80)); else echo $((a & 0x7f)); fi; a=$((a>>7)); done;
			ar | vu<u8>(a);
			CHECK(ar.data[0] == a);

			ar | vu<u8>(b);
			CHECK(ar.data[1] == char(0xff));
			CHECK(ar.data[2] == char(0x01));

			ar | vu<u8>(c);
			CHECK(ar.data[3] == char(0x80));
			CHECK(ar.data[4] == char(0x80));
			CHECK(ar.data[5] == char(0x02));

			ar | vu<u8>(d);
			CHECK(ar.data[6] == char(0x80));
			CHECK(ar.data[7] == char(0x92));
			CHECK(ar.data[8] == char(0xf4));
			CHECK(ar.data[9] == char(0x01));
		}

		SUBCASE("u16")
		{
			// a=32768; while [[ $a -ne 0 ]]; do if [[ $a -gt 32767 ]]; then echo $((a & 0x7fff|0x8000)); else echo $((a & 0x7fff)); fi; a=$((a>>15)); done;
			ar | vu<u16>(b);
			CHECK(ar.data[0] == char(0xff));
			CHECK(ar.data[1] == char(0x00));

			ar | vu<u16>(c);
			CHECK(ar.data[2] == char(0x00));
			CHECK(ar.data[3] == char(0x80));
			CHECK(ar.data[4] == char(0x01));
			CHECK(ar.data[5] == char(0x00));

			ar | vu<u16>(d);
			CHECK(ar.data[6] == char(0x00));
			CHECK(ar.data[7] == char(0x89));
			CHECK(ar.data[8] == char(0x7a));
			CHECK(ar.data[9] == char(0x00));
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

TEST_CASE("enum serialize")
{
	using namespace enum_types;
	using namespace pulmotor;
	pulmotor::archive_vector_out ar;

	A a{A1};
	S s{S0};
	C c{C::C0};
	X x{X::X0};

	ar | a | s | c | x;
	archive_vector_in i(ar.data);

	A a1;
	S s1;
	C c1;
	X x1;
	i | a1 | s1 | c1 | x1;

	CHECK(a1 == a);
	CHECK(s1 == s);
	CHECK(c1 == c);
	CHECK(x1 == x);
}

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
}

TEST_CASE("ptr serialize")
{
	using namespace ptr_types;
	using namespace pulmotor;
	pulmotor::archive_vector_out ar;

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
		std::aligned_storage<sizeof(A)> a[1];
		new (a) A(12345678);
		ar | place<A>(a) | x;

		std::aligned_storage<sizeof(A)> aa[1];
		archive_vector_in i(ar.data);
		i | place<A>(aa) | xx;

		CHECK(x == xx);

		A& a1 = *reinterpret_cast<A*>(a);
		A& a2 = *reinterpret_cast<A*>(aa);

		CHECK(a1.x == a2.x);
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

#include <pulmotor/std/vector.hpp>
TEST_CASE("vector")
{
	using namespace ptr_types;
	using namespace pulmotor;

	archive_vector_out ar;

	SUBCASE("empty")
	{
		std::vector<A> v;
		ar | v;

		std::vector<A> x;
		archive_vector_in i(ar.data);
		i | x;
		CHECK(x.empty());
	}

	SUBCASE("int")
	{
		std::vector<int> v{10, 20, 30, 40};
		ar | v;

		archive_vector_in i(ar.data);
		std::vector<int> x;
		i | x;

		CHECK(x.size() == v.size());
		CHECK(x == v);
	}

	SUBCASE("int ptr")
	{
		std::vector<int*> v{new int(10), new int(20)};
		ar | v;

		archive_vector_in i(ar.data);
		std::vector<int*> x;
		i | x;

		CHECK(x != v);
		CHECK(v.size() == x.size());
		CHECK(v.size() == x.size());
		for(auto z : v) delete z;
		for(auto z : x) delete z;
	}

	SUBCASE("struct")
	{
		std::vector<X> v{X{10}, X{20}, X{30}, X{40}};
		ar | v;

		archive_vector_in i(ar.data);
		std::vector<X> x;
		i | x;

		CHECK(x.size() == v.size());
		CHECK(x == v);
	}

	SUBCASE("struct ctor")
	{
		std::vector<A> v{A{10}, A{20}, A{30}, A{40}};
		ar | v;

		archive_vector_in i(ar.data);
		std::vector<A> x;
		i | x;

		CHECK(x.size() == v.size());
		CHECK(x == v);
	}

	SUBCASE("struct ptr ctor")
	{
		std::vector<A*> v{new A{15}, new A{25}};
		ar | v;

		archive_vector_in i(ar.data);
		std::vector<A*> x;
		i | x;

		CHECK(x.size() == v.size());
		CHECK((std::equal(x.begin(), x.end(), v.begin(), v.end(), [](auto a, auto b) { return *a == *b; })) == true);
		for(auto z : v) delete z;
		for(auto z : x) delete z;
	}
}

#include <pulmotor/std/string.hpp>
static pulmotor::romu3 r3;
TEST_CASE("string")
{
	using namespace pulmotor;

	archive_vector_out ar;

	SUBCASE("empty")
	{
		std::string s;
		ar | s | s | s;

		std::string x, y("hog"), z(128, 'c');
		archive_vector_in i(ar.data);
		i | x | y | z;
		CHECK(x.empty());
		CHECK(y.empty());
		CHECK(z.empty());
	}

	SUBCASE("content")
	{
		std::string ss("abcd"), sl("1234567890qwertyuiopasdfghjklzxcvbnmQWERTYUIOPASDFGHJKLZXCVBNM");
		ar | ss | sl;

		std::string xs, xl;
		archive_vector_in i(ar.data);
		i | xs | xl;

		CHECK(xs == ss);
		CHECK(xl == sl);
	}

	SUBCASE("various lengths")
	{
		constexpr size_t N=100;
		std::string sa[N], xa[N];
		r3.reset();
		for (int i=0; i<N; ++i)
			for(int q=0; q<i; ++q)
				sa[i] += '0' + r3.r(128-38);

		for (int i=0;i<1; ++i)
		{
			ar.data.clear();
			ar | sa;

			archive_vector_in in(ar.data);
			in | xa;

			CHECK((std::equal(sa, sa + N, xa, xa + N, [](auto a, auto b) { return a == b; })) == true);

			std::shuffle(sa, sa + N, r3);
		}
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



