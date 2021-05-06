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

struct A {
	int x;

	template<class Ar> void serialize(Ar& ar, unsigned version) { ar | x; }
};

namespace detect_serialize
{

struct X1 { }; template<class Ar> void serialize(Ar& ar, X1& x) { }
struct X2 { template<class Ar> void serialize(Ar& ar) { } };
struct X3 { }; template<class Ar> void serialize(Ar& ar, X3& x, unsigned version) { }
struct X4 { template<class Ar> void serialize(Ar& ar, unsigned version) { } };

//struct X5 { }; template<class Ar> X5* load_construct(Ar& ar, unsigned) { }
struct X6 { template<class Ar> static X6* load_construct(Ar& ar, unsigned version) { } };
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
	//CHECK(access::detect<Ar>::has_load_construct<T>::value == ((CHECKS & LC) != 0));
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

TEST_CASE("serialize")
{
	using namespace pulmotor;

	std::stringstream ss;
	pulmotor::sink_ostream so(ss);
	pulmotor::sink_archive ar(so);

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

	SUBCASE("struct")
	{
		A a{1234};

		archive_vector_out ar;
	//	ar | a;

		archive_vector_in i(ar.data);
		A x;
	//	i | x;

		//CHECK( a == x);

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
