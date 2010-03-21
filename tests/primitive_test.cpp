#include <pulmotor/ser.hpp>
#include <pulmotor/stream.hpp>
#include <pulmotor/util.hpp>
#include <string>


struct A
{
	int x, y;
	template<class ArchiveT>
	void serialize (ArchiveT& ar, unsigned version)
	{
		ar | x | y;
	}
};

int main ()
{
	using namespace pulmotor;

	{	
		printf ("TEST1\n");
		blit_section bs;

		int v = 10;

		bs | v;
		bs.dump_gathered ();
		
		std::vector<unsigned char> buf, bufx;
		bs.write_out (buf, pulmotor::is_be ());
		bs.write_out (bufx, pulmotor::is_le ());
		printf ("little-endian\n");
		pulmotor::util::hexdump (&*buf.begin (), buf.size());

		printf ("big-endian\n");
		pulmotor::util::hexdump (&*bufx.begin (), bufx.size());
	}

	{
		printf ("TEST2\n");
		blit_section bs;

		A a;
		a.x = 10;
		a.y = 20;

		bs | a;
		bs.dump_gathered ();
		
		std::vector<unsigned char> buf;
		bs.write_out (buf, false);
		pulmotor::util::hexdump (&*buf.begin (), buf.size());
	}

	{
		printf ("TEST3\n");
		blit_section bs;

		int ii [2];
		ii [0] = 0x11112222;
		ii [1] = 0x33445566;
		
		bs | ii;
		bs.dump_gathered ();
		
		std::vector<unsigned char> buf;
		bs.write_out (buf, false);
		pulmotor::util::hexdump (&*buf.begin (), buf.size());
	}

	{
		printf ("TEST4\n");
		blit_section bs;

		A aa [2];
		aa[0].x = 0x0001AAAA;
		aa[0].y = 0x0101BBBB;
		aa[1].x = 0x0202AAAA;
		aa[1].y = 0x0302BBBB;
		
		bs | aa;
		bs.dump_gathered ();
		
		std::vector<unsigned char> buf;
		bs.write_out (buf, true);
		pulmotor::util::hexdump (&*buf.begin (), buf.size());
	}

	{
		printf ("TEST5\n");
		blit_section bs;

		A* pa = new A ();
		pa->x = 0xAABBCCDD;
		pa->y = 0x22222222;
		
		bs | pa;
		bs.dump_gathered ();
		
		std::vector<unsigned char> buf;
		bs.write_out (buf, true);
		pulmotor::util::hexdump (&*buf.begin (), buf.size());
	}

	// pointer to pointers
	{
		printf ("TEST6\n");
		blit_section bs;

		A** pa = new A*[2];
		pa[0] = new A ();
		pa[0]->x = 0x0000AABB;
		pa[0]->y = 0x1111AABB;
		pa[1] = new A ();
		pa[1]->x = 0x2222AABB;
		pa[1]->y = 0x3333AABB;
		
		bs | ptr (pa, 2);
		bs.dump_gathered ();
		
		std::vector<unsigned char> buf;
		bs.write_out (buf, true);
		pulmotor::util::hexdump (&*buf.begin (), buf.size());
	}


	return 1;
}

