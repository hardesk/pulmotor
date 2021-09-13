#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <pulmotor/pulmotor_types.hpp>
#include <pulmotor/util.hpp>
#include <string>
using namespace std::string_literals;


#define TEST_ID1 dog
#define PRINT_ARG1(x) "one:" PULMOTOR_PSTR(x)
#define PRINT_ARG2(x, y) "two:" PULMOTOR_PSTR(x) ":" PULMOTOR_PSTR(y)

namespace pulmotor::util {
doctest::String toString(pulmotor::util::text_location const& sl)
{
	char buf[32];
	snprintf(buf, sizeof buf, "%u:%u", (unsigned)sl.line, (unsigned)sl.column);
	return buf;
}
}

TEST_CASE("pulmotor macros")
{
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
		map( [&r] (auto const& a) { return a; }, std::tuple<>() );
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

TEST_CASE("location map")
{
	SUBCASE("basic")
	{
		char const s0[] = "";
		char const s1[] = "\n";
		char const s2[] = "1\n2\n";
		char const s3[] = "x1x\nyy2yy";
		char const s4[] = "\nxxx2";

		auto check = [] (std::string_view s, char ch, pulmotor::util::text_location l) {

			auto gp = [](std::string_view s, char ch) -> size_t {
				size_t p = s.find_first_of(ch);
				return p == std::string::npos ? 0 : p;
			};

			pulmotor::util::location_map m0 (s.data(), s.size());
			m0.analyze();
			CHECK( m0.lookup(gp(s, ch)) == l);

			pulmotor::util::location_map m (s.data(), s.size());
			CHECK( m.lookup( gp(s, ch)) == l );
		};

		using sl = pulmotor::util::text_location;

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
		using namespace pulmotor::util;

		struct info { size_t offset; pulmotor::util::text_location loc; };

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
				size_t index = r3.r(inf.size());
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
				pulmotor::util::location_map l (sa[i].data(), sa[i].size());
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
				pulmotor::util::location_map l (sa[i].data(), sa[i].size());
				check(l, ia[i]);
			}

			SUBCASE("clean, random") {
				pulmotor::util::location_map l (sa[i].data(), sa[i].size());
				for (int g=0; g<16; ++g)
					check_random(g, l, ia[i]);
			}
		}

	}

}