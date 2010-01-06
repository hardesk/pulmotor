#include "common_test.hpp"

// A to B and B to A
struct A;
struct B;

struct A
{
	int v1;
	B*	pb;
	int v2;
	
	template<class ArchiveT>
	void blit (ArchiveT& ar, unsigned version)
	{
		ar | v1 | pb | v2;
	}
};

struct B
{
	int v1;
	A*	pa;
	int v2;
	
	template<class ArchiveT>
	void blit (ArchiveT& ar, unsigned version)
	{
		ar | v1 | pa | v2;
	}
};

// self reference
struct S
{
	int	value1;
	S*	ps;
	int	value2;

	template<class ArchiveT>
	void blit (ArchiveT& ar, unsigned version)
	{
		ar | value1 | ps | value2;
	}
};

struct Q
{
	int	value1;
	Q*	pq;
	int	value2;

	template<class ArchiveT>
	void blit (ArchiveT& ar, unsigned version)
	{
		ar | value1 | pq | value2;
	}
};

template<class T>
void blit_container (T& a, std::vector<unsigned char>& data)
{
	pulmotor::blit_section bs;
	blit_object (bs, a);
	
	bs.write_out (data);
}

int main ()
{
/*	{
		A a;
		B b;
		
		a.v1 = 0xaaaaaaa1;
		a.pb = &b;
		a.v2 = 0xaaaaaaa2;
		
		b.v1 = 0xbbbbbbb1;
		b.pa = &a;
		b.v2 = 0xbbbbbbb2;

		std::vector<unsigned char> odata;

		pulmotor::util::blit_to_container (a, odata, false);
		
		A* ra = pulmotor::util::fixup_pointers<A> (pulmotor::util::get_bsi (&*odata.begin (), false));
		
		assert (ra->v1 == a.v1);
		assert (ra->pb->v1 == b.v1);
		assert (ra->pb->pa->v1 == a.v1);
		assert (ra->v2 == a.v2);

		memset (&a, 0, sizeof (a));
		memset (&b, 0, sizeof (b));

		assert (ra->pb != 0);
		assert (ra->pb->pa != 0);
	}*/
	
	{
		S s1, s2, s3;
		s1.value1 = s1.value2 = 1;
		s1.ps = &s2;

		s2.value1 = s2.value2 = 2;
		s2.ps = &s3;

		s3.value1 = s3.value2 = 3;
		s3.ps = &s1;
		
		std::vector<unsigned char> odata;
		
		pulmotor::util::blit_to_container (s1, odata, false);
		
		S* rs = pulmotor::util::fixup_pointers<S> (pulmotor::util::get_bsi (&*odata.begin (), false));

		// nuke originals
		memset (&s1, 0, sizeof(s1));
		memset (&s2, 0, sizeof(s2));
		memset (&s3, 0, sizeof(s3));
		
		assert (rs->ps->ps->ps == rs);
 		assert (rs->value1 == 1 && rs->value2 == 1);
 		assert (rs->ps->value1 == 2 && rs->ps->value2 == 2);
 		assert (rs->ps->ps->value2 == 3 && rs->ps->ps->value2 == 3);
 		assert (rs->ps->ps->ps->value2 == 1 && rs->ps->ps->ps->value2 == 1);
	}

/*	{
		Q q;
		q.value1 = 1;
		q.pq = &q;
		q.value2 = 1;
		
		std::vector<unsigned char> odata;
		
		pulmotor::util::blit_to_container (q, odata, false);
		
		Q* rq = pulmotor::util::fixup_pointers<Q> (pulmotor::util::get_bsi (&*odata.begin (), false));
		
		assert (rq->pq == rq);
		assert (rq->pq->pq == rq);

		memset (&q, 0, sizeof(q));
		assert (rq->pq == rq);
	}*/
		
}
