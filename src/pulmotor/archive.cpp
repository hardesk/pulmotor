#include "archive.hpp"

#include "serialize.hpp"
#include "binary_archive.hpp"

namespace pulmotor
{

char null_32[32] = { 0 };

//char null_32[32] = { '-', '-', '-', '-', '-', '-', '-', '-', '-', '-', '-', '-', '-', '-', '-', '-',
// '-', '-', '-', '-', '-', '-', '-', '-', '-', '-', '-', '-', '-', '-', '-', '-' };


}


#include "std/string.hpp"



extern pulmotor::XXX xx;
void serialize_test1(pulmotor::archive_memory& ar, std::string const& s)
{
    int a = 10;
    // std::string x = "world";
    // ar | x | a | s | x | s | a | a | a | a | a;

    pulmotor::XXX& x = xx;
    ar | x.a | x.b | x.c | x.d | x.e | x.f | x.g;
    // xx = x;
}
