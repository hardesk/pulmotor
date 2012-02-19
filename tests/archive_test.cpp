#include <pulmotor/archive.hpp>
#include <pulmotor/stream.hpp>


//template<class T> struct track_version {};

//PULMOTOR_ARCHIVE_NOVER(std::string);
namespace pulmotor {

PULMOTOR_ARCHIVE_FREE_SPLIT(std::string)
PULMOTOR_ARCHIVE_READ(std::string)
{
	pulmotor::u32 size = 0;
	ar & size;
	v.resize (size);
	ar & pulmotor::memblock (v.c_str(), size);
}
PULMOTOR_ARCHIVE_WRITE(std::string)
{
	ar & (pulmotor::u32)v.size ();
	ar & pulmotor::memblock (v.c_str(), v.size ());
}

}

struct A
{
	int x;
	std::string name;
	std::string m[3];

	A(bool doInit)
   	{
		if (doInit) {
			x = 0x00112233;
			name = "Sandy Black";

			m[0] = "member 1";
			m[1] = "member 2";
			m[2] = "member 3";
		}
	}
	PULMOTOR_ARCHIVE()
	{
		ar & x & name & m;
	}
};

int main ()
{
	std::auto_ptr<pulmotor::basic_output_buffer> oab = pulmotor::create_plain_output ("test.o.1");
	
	{
		pulmotor::output_archive oa (*oab);
		A a (true);
		archive (oa, a);
	}
	oab.reset ();
	
	// Try reading it now!
	std::auto_ptr<pulmotor::basic_input_buffer> iab = pulmotor::create_plain_input ("test.o.1");
	{
		pulmotor::input_archive ia (*iab);
		A a (false);
		archive (ia, a);

		A a1 (true);
		assert (a.x == a1.x);
		assert (a.name == a1.name);
		assert (a.m[0] == a1.m[0]);
		assert (a.m[1] == a1.m[1]);
		assert (a.m[2] == a1.m[2]);
	}

	return 1;
}
