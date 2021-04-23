#include "common_test.hpp"

struct X;

struct A
{
	size_t	size;
	X*		px;
	
	template<class ArchiveT>
	void serialize (ArchiveT& ar, unsigned version)
	{
		ar	| pulmotor::ptr (px, size);
	}
};

struct X
{
	int value;

	template<class ArchiveT>
	void serialize (ArchiveT& ar, unsigned version)
	{
		ar	| value;
	}	
};

struct B
{
	X	xa[10];

	template<class ArchiveT>
	void serialize (ArchiveT& ar, unsigned version)
	{
		ar	| xa;
	}
};

/*struct C
{
	X*	xpa[10];

	template<class ArchiveT>
	void blit (ArchiveT& ar, unsigned version)
	{
		ar	| pulmotor::array (xpa);
	}
};*/

int main ()
{

	// pointer to an array
	{
		A a;
		a.size = 8;
		a.px = new X [a.size];
		
		for (size_t i=0; i<a.size; ++i)
			a.px [i].value = i;
			
		std::vector<unsigned char> odata;

		pulmotor::util::blit_to_container (a, odata, pulmotor::target_traits::be_lp32);
		
		A* ra = pulmotor::util::fixup_pointers<A> (pulmotor::util::get_bsi (&*odata.begin (), false));

		assert (ra->size == 8);
		for (size_t i=0; i<8; ++i)
			assert ((size_t)ra->px [i].value == i);
	}

	// array of structs
	{
		B b;		
		for (size_t i=0; i<sizeof(b.xa) / sizeof(b.xa[0]); ++i)
			b.xa [i].value = i;
		
		std::vector<unsigned char> odata;
		pulmotor::util::blit_to_container (b, odata, pulmotor::target_traits::be_lp32);
		
		B* rb = pulmotor::util::fixup_pointers<B> (pulmotor::util::get_bsi (&*odata.begin (), false));
		
		for (size_t i=0; i<sizeof(rb->xa) / sizeof(rb->xa[0]); ++i)
			assert ((size_t)rb->xa[i].value == i);
	}
	
	return 0;
}
