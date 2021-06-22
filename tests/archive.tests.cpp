#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>
#include <pulmotor/archive.hpp>

using namespace pulmotor;
static romu3 r3;

struct A
{
	int a;
	long b;
	long long c;

	template<class ArchiveT>
	//void archive(ArchiveT& ar) { ar | mknv(a) | mknv(b) | mknv(c); }
	void archive(ArchiveT& ar, unsigned version) { ar | a | b | c; }
};

template<class T>
struct ar_check
{
	using v_t = std::vector<char>;

	static T one(v_t& v) {
		T a = *(T const*)&v[0];
		return a;
	}
	static T pull(v_t& v, size_t& offset) {
		T a = *(T const*)&v[offset];
		offset += sizeof(T);
		return a;
	}
	static std::string pull_str(v_t& v, size_t& offset) {
		T l = *(T const*)&v[offset];
		offset += sizeof(l);

		std::string ss(&v[offset], l);
		offset += l;
		return ss;
	}

};


TEST_CASE("pulmotor archive")
{
	SUBCASE("write basic")
	{
		pulmotor::archive_vector_out ar;

		ar.write_basic('A');
		ar.write_basic('X');
		CHECK(ar.str() == "AX");
	}

	SUBCASE("write block")
	{
		std::stringstream ss;
		pulmotor::sink_ostream so(ss);
		pulmotor::archive_sink ar(so);

		char data[]="hello";
		ar.write_data(data, strlen(data));
		CHECK(!ar.ec);

		CHECK(ss.str() == data);
	}

	SUBCASE("version")
	{
		struct A { int x; };
		A a{10};

		SUBCASE("basic")
		{
			pulmotor::archive_vector_out ar;
			ar.write_object_prefix(&a, 1337);
			CHECK(ar_check<u32>::one(ar.data) == 1337);
		}

		SUBCASE("typename")
		{
			pulmotor::archive_vector_out ar(ver_flag_debug_string);
			ar.write_object_prefix(&a, 1337);

			size_t offset = 0;
			CHECK((ar_check<u32>::pull(ar.data, offset) & ver_mask) == 1337);
			CHECK(ar_check<u32>::pull_str(ar.data, offset) == typeid(A).name());
		}

		size_t al = archive_vector_out::forced_align;
		SUBCASE("forced align")
		{
			pulmotor::archive_vector_out ar(ver_flag_align_object);
			ar.write_object_prefix(&a, 1337);
			CHECK(ar.offset() == al);

			size_t offset = 0;
			CHECK((ar_check<u32>::pull(ar.data, offset) & ver_mask) == 1337);
			size_t garbage = ar_check<u32>::pull(ar.data, offset);
			CHECK( (offset + garbage) == al);
		}

		SUBCASE("forced string+align")
		{
			pulmotor::archive_vector_out ar(ver_flag_debug_string|ver_flag_align_object);
			ar.write_object_prefix(&a, 1337);
			CHECK(ar.offset() == al);

			size_t offset = 0;
			CHECK((ar_check<u32>::pull(ar.data, offset) & ver_mask) == 1337);
			size_t garbage = ar_check<u32>::pull(ar.data, offset);
			CHECK(offset + garbage == al);
			CHECK(ar_check<u32>::pull_str(ar.data, offset) == typeid(A).name());
		}
	}
}

TEST_CASE("pulmotor archive version")
{
}

TEST_CASE("size packing")
{
	auto TC=[](int N, auto* pd) {
		int r=0;
		size_t s = 0;
		int state =0, i=0;

		while(i<12) {
			bool cont = util::duleb(s, state, pd[i++]);
			//CHECK(cont == (i<N));
			if (!cont) break;
		}
		CHECK(i==N);
		return s;
	};

	SUBCASE("u8") {
		u8 b[12];
		CHECK(util::euleb(0, b) == 1);
		CHECK(b[0]==0);
		CHECK(TC(1, b) == 0);

		CHECK(util::euleb(1, b) == 1);
		CHECK(b[0]==1);
		CHECK(TC(1, b) == 1);

		CHECK(util::euleb(128, b) == 2);
		CHECK(b[0]==0x80);
		CHECK(b[1]==0x01);
		CHECK(TC(2, b) == 128);

		// 7fff ->  0111 1111    1111 1111
		//		    0111 1111 1  M111 1111
		//		   01 M111 1111  M111 1111
		//		M..01 M111 1111  M111 1111
		CHECK(util::euleb(32767, b) == 3);
		CHECK(b[0]==0xff);
		CHECK(b[1]==0xff);
		CHECK(b[2]==0x01);
		CHECK(TC(3, b) == 32767);
	}

	SUBCASE("u16") {
		u16 b[12];

		CHECK(util::euleb(1, b) == 1);
		CHECK(TC(1, b) == 1);

		CHECK(util::euleb(0x7fff, b) == 1);
		CHECK(b[0]==0x7fff);
		CHECK(TC(1, b) == 0x7fff);

		CHECK(util::euleb(0x8000, b) == 2);
		CHECK(b[0]==0x8000);
		CHECK(b[1]==0x0001);
		CHECK(TC(2, b) == 0x8000);

		CHECK(util::euleb(0x82345678, b) == 3);
		CHECK(TC(3, b) == 0x82345678);
	}

	SUBCASE("u32") {
		u32 b[4];

		CHECK(util::euleb(1, b) == 1);
		CHECK(TC(1, b) == 1);

		CHECK(util::euleb(0x7fff'ffff, b) == 1);
		CHECK(b[0]==0x7fff'ffff);
		CHECK(TC(1, b) == 0x7fff'ffff);

		CHECK(util::euleb(0x8000'ffff, b) == 2);
		CHECK(b[0]==0x8000'ffff);
		CHECK(b[1]==0x0001);
		CHECK(TC(2, b) == 0x8000'ffff);

		size_t ss = 0xfedc'ba98'7654'3210u;
		CHECK(util::euleb(ss, b) == 3);
		CHECK(TC(3, b) == ss);
	}
}
