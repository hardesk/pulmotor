#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <pulmotor/pulmotor_types.hpp>
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
