#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>

#include <pulmotor/stream.hpp>

#define T_N "pulmotor.stream.test.data"
#define T_S 16384

pulmotor::romu3 r3;

bool make_temp_file(pulmotor::path_char const* path, size_t size)
{
	std::fstream f(path, std::ios_base::out|std::ios_base::trunc);
	for(int i=0; i<size; ++i)
		f << char('a' + r3.r(26));
	return f.good();
}

TEST_CASE("stream input")
{
	CHECK(make_temp_file(T_N, T_S));

	std::error_code ec;
	std::string init;
	{
		std::fstream f(T_N);
		f >> init;
	}

	pulmotor::fs_t fs = pulmotor::file_size(T_N);
	size_t ps = pulmotor::get_pagesize();
	CHECK( fs == T_S );
	CHECK( (T_S % ps) == 0);

	auto check_full = [&](pulmotor::source& ss)
	{
		char* buffer = new char[T_S];
		CHECK(ss.offset()==0);
		size_t fetched=ss.fetch(buffer, T_S, ec);
		CHECK(!ec);
		CHECK(ss.offset() == T_S);
		CHECK(ss.avail() == 0);
		CHECK(fetched == T_S);
		CHECK(memcmp(buffer, init.data(), T_S) == 0);
		delete[] buffer;
	};
	
	auto check_chunked = [&](pulmotor::source& ss)
	{
		size_t bs=ps/2;
		char* buffer = new char[bs];
		for (size_t read=0; read<T_S; )
		{
			size_t fetched=ss.fetch(buffer, bs, ec);
			CHECK(!ec);
			CHECK(ss.offset() == read+bs);
			CHECK(fetched == bs);
			CHECK(memcmp(buffer, init.data() + read, bs) == 0);
			read+=fetched;
		}
		delete[] buffer;
	};

	SUBCASE("full istream")
	{
		std::ifstream is(T_N);
		pulmotor::source_istream ss(is);

		CHECK(ss.size() == T_S);
		CHECK(ss.avail() == 0);
		ss.prefetch();
		CHECK(ss.avail() == T_S);

		check_full(ss);
	}

	SUBCASE("full mmap")
	{
		pulmotor::source_mmap m;
		m.map(T_N, pulmotor::source_mmap::ro, ec);

		CHECK(m.size() == T_S);
		CHECK(m.avail() == T_S);
		CHECK(memcmp(m.data(), init.data(), T_S) == 0);

		check_full(m);
	}

	SUBCASE("chunk istream")
	{
		std::ifstream is(T_N);
		pulmotor::source_istream ss(is, ps);

		CHECK(ss.size() == T_S);
		CHECK(ss.avail() == 0);
		ss.prefetch();
		CHECK(ss.avail() == ps);

		check_chunked(ss);
	}

	SUBCASE("chunk mmap")
	{
		pulmotor::source_mmap m;
		m.map(T_N, pulmotor::source_mmap::ro, ec, 0, ps);

		CHECK(m.size() == T_S);
		CHECK(m.avail() == ps);

		check_chunked(m);
	}

	unlink(T_N);
}
