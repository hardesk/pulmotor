#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <pulmotor/pulmotor_types.hpp>
#include <pulmotor/util.hpp>
#include <string>
using namespace std::string_literals;


#define TEST_ID1 dog
#define PRINT_ARG1(x) "one:" PULMOTOR_PSTR(x)
#define PRINT_ARG2(x, y) "two:" PULMOTOR_PSTR(x) ":" PULMOTOR_PSTR(y)

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
		auto r = map( [] (int& a) -> int& { return a; }, a );
		std::get<0>(r) += 2;
		std::get<1>(r) += 3;

		CHECK(std::get<0>(a) == 12);
		CHECK(std::get<1>(a) == 23);
	}
}

