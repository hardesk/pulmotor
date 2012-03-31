#include <pulmotor/pulmotor_fwd.hpp>
#include <pulmotor/archive.hpp>
#include <pulmotor/stream.hpp>
#include <pulmotor/archive_std.hpp>
#include <pulmotor/ser.hpp>

#include <list>
#include <vector>
#include <iostream>

//template<class T> struct track_version {};

struct MyCont
{
	int value;
};

template<class ArchiveT>
void archive (ArchiveT& ar, MyCont& v, unsigned version)
{
	ar & v.value;
}

void serialize (pulmotor::blit_section& ar, MyCont& v, unsigned version)
{
	ar | v.value;
}

struct A
{
	int x;
	float ff;
	char c;
	float ffa[2];

	std::string name;
	std::string m[3];

	std::vector<int> vi;
	std::vector<std::string> vs;
	std::vector<std::vector<MyCont> > vvm;

	A(bool doInit)
   	{
		if (doInit) {
			x = 0x00112233;
			c = 'X';
			ff = 2.0f;
			ffa[0] = 20.0f;
			ffa[1] = 40.0f;
			name = "Sandy Black";

			m[0] = "member 1";
			m[1] = "member 2";
			m[2] = "member 3";

			vi.push_back (0x11223344);
			vi.push_back (0x55667788);

			vs.push_back ("one");
			vs.push_back ("two");
			vs.push_back ("three");
			
			vvm.push_back (std::vector<MyCont>());
			MyCont mc1 = { 100 };
			vvm.back().push_back (mc1);
		}
	}
	PULMOTOR_ARCHIVE()
	{
		ar & x & ff & c & ffa & name & m & vi & vs & vvm;
	}
};

void serialize (pulmotor::blit_section& ar, A& a, unsigned version)
{
	ar | a;
}

int main ()
{
	using namespace pulmotor;

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
		memset (&a, 0xff, (char*)&a.name - (char*)&a);
		archive (ia, a);

		A a1 (true);

		assert (a.x == a1.x);
		assert (a.ff == a1.ff);
		assert (a.c == a1.c);
		assert (a.ffa[0] == a1.ffa[0]);
		assert (a.ffa[1] == a1.ffa[1]);
		assert (a.name == a1.name);
		assert (a.m[0] == a1.m[0]);
		assert (a.m[1] == a1.m[1]);
		assert (a.m[2] == a1.m[2]);
		assert (a.vi == a1.vi);
		assert (a.vs == a1.vs);
	}

	return 1;
}
