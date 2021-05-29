#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>
#include <pulmotor/archive.hpp>

using namespace pulmotor;
romu3 r3;

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
			CHECK((ar_check<u32>::pull(ar.data, offset) & ver_flag_mask) == 1337);
			CHECK(ar_check<u32>::pull_str(ar.data, offset) == typeid(A).name());
		}

		size_t al = archive_vector_out::forced_align;
		SUBCASE("forced align")
		{
			pulmotor::archive_vector_out ar(ver_flag_align_object);
			ar.write_object_prefix(&a, 1337);
			CHECK(ar.offset() == al);

			size_t offset = 0;
			CHECK((ar_check<u32>::pull(ar.data, offset) & ver_flag_mask) == 1337);
			size_t garbage = ar_check<u32>::pull(ar.data, offset);
			CHECK( (offset + garbage) == al);
		}

		SUBCASE("forced string+align")
		{
			pulmotor::archive_vector_out ar(ver_flag_debug_string|ver_flag_align_object);
			ar.write_object_prefix(&a, 1337);
			CHECK(ar.offset() == al);

			size_t offset = 0;
			CHECK((ar_check<u32>::pull(ar.data, offset) & ver_flag_mask) == 1337);
			size_t garbage = ar_check<u32>::pull(ar.data, offset);
			CHECK(offset + garbage == al);
			CHECK(ar_check<u32>::pull_str(ar.data, offset) == typeid(A).name());
		}
	}
}

TEST_CASE("pulmotor archive version")
{
}

