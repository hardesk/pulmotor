#include "archive.hpp"

namespace pulmotor
{

char null_32[32] = { 0 };

//char null_32[32] = { '-', '-', '-', '-', '-', '-', '-', '-', '-', '-', '-', '-', '-', '-', '-', '-',
// '-', '-', '-', '-', '-', '-', '-', '-', '-', '-', '-', '-', '-', '-', '-', '-' };

}


#include "serialize.hpp"
#include "binary_archive.hpp"
#include "std/string.hpp"

void serialize_test1(pulmotor::archive_sink& ar, std::string const& s)
{
	int a = 100;
	std::string x = "world";
	ar | x | a | s | x | s | a | a | a | a | a;
}

