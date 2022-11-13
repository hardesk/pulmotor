#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>

#include <pulmotor/pulmotor_types.hpp>
#include <pulmotor/util.hpp>
#include <string>
using namespace std::string_literals;


#define TEST_ID1 dog
#define PRINT_ARG1(x) "one:" PULMOTOR_PSTR(x)
#define PRINT_ARG2(x, y) "two:" PULMOTOR_PSTR(x) ":" PULMOTOR_PSTR(y)

void test_yaml();

namespace pulmotor::util {
doctest::String toString(pulmotor::text_location const& sl)
{
	char buf[32];
	snprintf(buf, sizeof buf, "%u:%u", (unsigned)sl.line, (unsigned)sl.column);
	return buf;
}
}

TEST_CASE("pulmotor macros")
{
	test_yaml();

	CHECK(PULMOTOR_PSIZE() == 1);
	CHECK(PULMOTOR_PSIZE(a) == 1);
	CHECK(PULMOTOR_PSIZE(a,b,c) == 3);

	char buf[64];
	snprintf(buf, sizeof buf, "%s", PULMOTOR_PSTR(pig));
	CHECK(buf == "pig"s);

	snprintf(buf, sizeof buf, "%s", PULMOTOR_PSTR(TEST_ID1));
	CHECK(buf == "dog"s);

	snprintf(buf, sizeof buf, "%s", PULMOTOR_POVERLOAD(PRINT_ARG, A));
	CHECK(buf == "one:A"s);

	snprintf(buf, sizeof buf, "%s", PULMOTOR_POVERLOAD(PRINT_ARG, A, B));
	CHECK(buf == "two:A:B"s);
}

TEST_CASE("map tuple")
{
	using namespace pulmotor::util;

	SUBCASE("empty")
	{
		int r = 0;
		map( [] (auto const& a) { return a; }, std::tuple<>() );
		CHECK(r == 0);
	}

	SUBCASE("void")
	{
		int r = 0;
		map( [&r] (auto const& a) { r += a; }, std::tuple(10, 20) );
		CHECK(r == 30);
	}

	SUBCASE("value")
	{
		auto r = map( [] (auto const& a) { return a + 1; }, std::tuple(10, 20) );
		CHECK(std::get<0>(r) == 11);
		CHECK(std::get<1>(r) == 21);
	}

	SUBCASE("reference")
	{
		std::tuple<int, int> a(10, 20);
		auto r = map( [] (int& x) -> int& { return x; }, a );
		std::get<0>(r) += 2;
		std::get<1>(r) += 3;

		CHECK(std::get<0>(a) == 12);
		CHECK(std::get<1>(a) == 23);
	}
}

TEST_CASE("type at_i")
{
	CHECK( (std::is_same<pulmotor::util::at_i<0, int, float, char>::type, int>::value) == true);
	CHECK( (std::is_same<pulmotor::util::at_i<1, int, float, char>::type, float>::value) == true);
	CHECK( (std::is_same<pulmotor::util::at_i<2, int, float, char>::type, char>::value) == true);
}

TEST_CASE("type list")
{
	struct X {};
	using namespace pulmotor;
	using l0 = tl::list<>;
	using l1 = tl::list<int>;
	using l2 = tl::list<int, float>;
	using l3 = tl::list<int, float, X>;

	CHECK( (tl::has<int, l0>::value) == false);
	CHECK( (tl::index<int, l0>::value) >= 0);

	CHECK( (tl::has<int, l1>::value) == true);
	CHECK( (tl::has<char, l1>::value) == false);
	CHECK( (tl::index<int, l1>::value) == 0);
	CHECK( (tl::index<char, l1>::value) >= 1);

	CHECK( (tl::index<int, l2>::value) == 0);
	CHECK( (tl::index<float, l2>::value) == 1);
	CHECK( (tl::index<char, l2>::value) >= 2);

	CHECK( (tl::has<int, l3>::value) == true);
	CHECK( (tl::has<float, l3>::value) == true);
	CHECK( (tl::has<X, l3>::value) == true);
	CHECK( (tl::has<char, l3>::value) == false);
	CHECK( (tl::index<int, l3>::value) == 0);
	CHECK( (tl::index<float, l3>::value) == 1);
	CHECK( (tl::index<X, l3>::value) == 2);
	CHECK( (tl::index<unsigned, l3>::value) >= 3);
}

TEST_CASE("scoped exit")
{
	bool s0 = false, s1 = false, s2 = false, s3 = false, s4 = false;

	pulmotor::scope_exit e( [&s0]() { s0 = true; } );
	CHECK( s0 == false );

	{
		pulmotor::scope_exit e( [&s1]() { s1 = true; } );
	}
	CHECK( s1 == true );

	{
		try {
			pulmotor::scope_exit e( [&s2]() { s2 = true; } );
			throw int(10);
		} catch(int e) {
			CHECK(s2 == true);
		}
	}
	CHECK( s2 == true );

	{
		pulmotor::scope_exit e( [&s3]() { s3 = true; } );
		e.release();
	}
	CHECK( s3 == false );

	{
		try {
			pulmotor::scope_exit e( [&s4]() { s4 = true; } );
			e.release();
			throw int(10);
		} catch(int e) {
			CHECK(s4 == false);
		}
	}
	CHECK( s4 == false );
}

TEST_CASE("location map")
{
	SUBCASE("basic")
	{
		char const s0[] = "";
		char const s1[] = "\n";
		char const s2[] = "1\n2\n";
		char const s3[] = "x1x\nyy2yy";
		char const s4[] = "\nxxx2";

		auto check = [] (std::string_view s, char ch, pulmotor::text_location l) {

			auto gp = [](std::string_view s, char ch) -> size_t {
				size_t p = s.find_first_of(ch);
				return p == std::string::npos ? 0 : p;
			};

			pulmotor::location_map m0 (s.data(), s.size());
			m0.analyze();
			CHECK( m0.lookup(gp(s, ch)) == l);

			pulmotor::location_map m (s.data(), s.size());
			CHECK( m.lookup( gp(s, ch)) == l );
		};

		using sl = pulmotor::text_location;

		check( s0, 'x', sl{1,1} );

		check( s1, 'x', sl{1,1} );

		check( s2, '1', sl{1,1} );
		check( s2, '2', sl{2,1} );

		check( s3, '1', sl{1,2} );
		check( s3, '2', sl{2,3} );

		check( s4, '2', sl{2,4} );
	}

	SUBCASE("full check")
	{
		using namespace pulmotor;

		struct info { size_t offset; pulmotor::text_location loc; };

		auto build_info = [](std::string_view s, std::vector<info>& o) {
			size_t line = 1, col = 1;
			for (size_t i=0; i<s.size(); ++i) {
				o.push_back( info {i, text_location { unsigned(line), unsigned(col++)} } );
				if (s[i] == '\n') { col = 1; line++; }
			}
		};

		auto check = [] (location_map& lm, std::vector<info> const& inf) {
			for(auto const& precomputed : inf) {
				text_location loc = lm.lookup( precomputed.offset );
				CHECK(loc == precomputed.loc);
			}
		};

		auto check_random = [] (size_t gen, location_map& lm, std::vector<info> const& inf) {
			pulmotor::romu3 r3;

			for(int i=gen*15; i --> 0; ) r3();

			for(size_t q=inf.size() * 4; q --> 0; ) {
				size_t index = r3.range(inf.size());
				assert(index < inf.size());
				auto const& precomputed = inf[index];
				text_location loc = lm.lookup( precomputed.offset );
				CHECK(loc == precomputed.loc);
			}
		};

		std::string_view sa[] = {
			"zxcvbmn\na\n\nbb\n",
			"\n\nzxcvbmn\na\n\nbb"
		};

		std::vector<info> ia[std::size(sa)];
		for (size_t i=0; i<std::size(sa); ++i)
		{
			build_info(sa[i], ia[i]);

			SUBCASE("analyze") {
				pulmotor::location_map l (sa[i].data(), sa[i].size());
				l.analyze();

				SUBCASE("iterate") {
					check(l, ia[i]);
				}

				SUBCASE("random") {
					for (int g=0; g<16; ++g)
						check_random(g, l, ia[i]);
				}
			}

			SUBCASE("clean, iterate") {
				pulmotor::location_map l (sa[i].data(), sa[i].size());
				check(l, ia[i]);
			}

			SUBCASE("clean, random") {
				pulmotor::location_map l (sa[i].data(), sa[i].size());
				for (int g=0; g<16; ++g)
					check_random(g, l, ia[i]);
			}
		}

	}
}

TEST_CASE("base64")
{
	using namespace pulmotor;

	char out[32];
	SUBCASE("decode empty")
	{
		out[0] = 0x7;
		size_t rl = util::base64_decode("", 0, out);
		CHECK(rl == 0);
		CHECK(out[0] == 0x7);

		rl = util::base64_decode(" ", 1, out);
		CHECK(rl == 0);
		CHECK(out[0] == 0x7);

		rl = util::base64_decode("  ", 2, out);
		CHECK(rl == 0);
		CHECK(out[0] == 0x7);

		rl = util::base64_decode("   ", 3, out);
		CHECK(rl == 0);
		CHECK(out[0] == 0x7);

		rl = util::base64_decode("    ", 4, out);
		CHECK(rl == 0);
		CHECK(out[0] == 0x7);

		rl = util::base64_decode("     ", 5, out);
		CHECK(rl == 0);
		CHECK(out[0] == 0x7);
	}

	struct T {
		char const* enc;
		char const* dec;
	} d[] = {
		// { "eA==", "x" }, // normal padding
		// { "eQ=", "y" }, // weird case
		// { "eg", "z" }, // no padding

		// { "eHk=", "xy" }, // padded
		// { "enc", "zw" }, // not padded

		// { "eHl6", "xyz" },

		{ "eHl6dw==", "xyzw" },
		{ "e\n\nH\nl\n\n\n6d\nw=\n=", "xyzw" },
		{ "e\t\t\nH\nl\n\n\n6d\nw=\n=", "xyzw" },
		{ "e\t\nHl  6dw=\n=\n\n", "xyzw" },
		{ "eg\n\n\n", "z" },
	};

	SUBCASE("decode some")
	{
		auto clean_str = [](std::string_view s) {
			std::string clean;
			std::copy_if(s.begin(), s.end(),
				std::back_insert_iterator(clean),
				[](char a) { return !std::isspace(a); } );
			return clean;
		};
		for (size_t i=0; i<sizeof d / sizeof d[0]; ++i) {
			size_t enc_len = strlen(d[i].enc);
			size_t src_len = strlen(d[i].dec);

			// check encode padded
			memset(out, 0, sizeof out);
			size_t encoded_len = util::base64_encode_length(src_len, util::base64_options::pad);
			util::base64_encode(d[i].dec, src_len, out, util::base64_options::pad);

			std::string_view enc_sv(d[i].enc, enc_len);
			if (enc_sv.find_first_of(" \t\n") == std::string_view::npos)
				CHECK( memcmp(d[i].enc, out, enc_len) == 0);
			else
			{
				std::string clean = clean_str(enc_sv);
				CHECK( clean.compare( 0, clean.size(), std::string_view(out, encoded_len), 0, clean.size() ) == 0 );
			}

			// check encode unpadded
			memset(out, 0, sizeof out);
			encoded_len = util::base64_encode_length(src_len, util::base64_options::nopad);
			util::base64_encode(d[i].dec, src_len, out, util::base64_options::nopad);
			std::string clean = clean_str(enc_sv);
			CHECK( encoded_len <= enc_len );
			CHECK( clean.compare( 0, encoded_len, std::string_view(out, encoded_len) ) == 0 );

			// check decode
			memset(out, 0, sizeof out);
			size_t dec_len = util::base64_decode(d[i].enc, enc_len, out);
			CHECK( dec_len == src_len );
			CHECK( memcmp(d[i].dec, out, dec_len) == 0);
		}
	}

	SUBCASE("decode random")
	{
		pulmotor::romu3 r;

		static const size_t SIZE = 4 * 256 * 2;
		size_t ENCODED_SIZE = pulmotor::util::base64_encode_length(SIZE);
		size_t DECODED_SIZE = pulmotor::util::base64_decode_length_approx(ENCODED_SIZE);

		char* content = new char[SIZE];
		char* encoded = new char[ENCODED_SIZE];
		char* decoded = new char[DECODED_SIZE];
		PULMOTOR_SCOPE_EXIT(&) { delete[] content; delete[] encoded; delete[] decoded; };

		size_t ii = 0;
		std::generate_n(content,			SIZE / 2, [&ii]() { return ii++ >> 2; });
		std::generate_n(content + SIZE / 2, SIZE / 2, [&r]() { return r.range(256); });

		pulmotor::util::base64_encode(content, SIZE, encoded, util::base64_options::pad);
		size_t decoded_size = pulmotor::util::base64_decode(encoded, ENCODED_SIZE, decoded);
		CHECK(decoded_size == SIZE);
		CHECK(memcmp(content, decoded, SIZE) == 0);
	}

}
