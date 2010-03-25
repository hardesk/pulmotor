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

struct C
{
	int z;
	A a;
	template<class ArchiveT>
	void serialize (ArchiveT& ar, unsigned version)
	{
		ar | z | a;
	}
};

struct B
{
	char* s;
	int x;
	//A a;
	//A aa[2];

	template<class ArchiveT>
	void serialize (ArchiveT& ar, unsigned version)
	{
		ar | pulmotor::ptr (s, strlen(s)+1) | x;
		//ar | pulmotor::ptr (s, strlen(s)+1) | x | a | aa;
		//ar | a | aa;
	}
};

struct P
{
	char* c;
	int* x;
	A* a;
	A* aa[2];

	P ()
	{
		c = "candle";

		x = new int [4];
		x [0] = 0x40;
		x [1] = 0x41;
		x [2] = 0x42;
		x [3] = 0x43;

		a = new A[2];
		a[0].x = 0x22220001;
		a[0].y = 0x22220101;
		a[1].x = 0x22220002;
		a[1].y = 0x22220102;

		aa[0] = new A [2];
		aa[1] = new A [3];

	}

	template<class ArchiveT>
	void serialize (ArchiveT& ar, unsigned version)
	{
		using namespace pulmotor;
		ar | ptr (c, strlen(c)+1)
		   | x | a
		   | ptr (aa[0], 2)
		   | ptr (aa[1], 3)
		   ;
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

	// composing composite
	{
		printf ("TEST7\n");
		blit_section bs;

		C c;

		c.z = 0x12341234;
		c.a.x = 0x000a1111;
		c.a.y = 0x000a2222;
		bs | c;
		bs.dump_gathered ();
		
		std::vector<unsigned char> buf;
		bs.write_out (buf, true);
		pulmotor::util::hexdump (&*buf.begin (), buf.size());
	}


	// composing composite
	{
		printf ("TEST8\n");
		blit_section bs;

		B b;

		b.s = "table";
		b.x = 0x12341234;
		//b.a.x = 0x000a1111;
		//b.a.y = 0x000a2222;
		//b.aa[0].x = 0xaaaa0011;
		//b.aa[0].y = 0xaaaa0022;
		//b.aa[1].x = 0xaaaa1111;
		//b.aa[1].y = 0xaaaa1122;

		bs | b;
		bs.dump_gathered ();
		
		std::vector<unsigned char> buf;
		bs.write_out (buf, true);
		pulmotor::util::hexdump (&*buf.begin (), buf.size());
	}


	return 1;
}

