#include <pulmotor/ser.hpp>
#include <pulmotor/stream.hpp>
#include <string>

namespace penis {

struct A_generic
{
	float	c;
	
	template<class ArchiveT>
	void blit (ArchiveT& ar, unsigned version) {
		ar | c;
	}
};

struct A_target
{
	char	c;
	int*	p[2];

	template<class ArchiveT>
	void blit (ArchiveT& ar, unsigned version) {
		ar | c | p;
	}
};

} // penis


struct Data
{
	int			count;
	A_generic*	pa;
	
	Data (int c)
	:	count (c) {
		pa = new A_generic [c];
		
		for (int i=0; i<c; ++i)
			pa[i] = i;
	}
	
	template<class ArchiveT>
	void blit (ArchiveT& ar, unsigned version) {
		using pulmotor::ptr;
		using pulmotor::array;
		
		ar	
			| count
			;
			
		if (ar.is_target (version) == pulmotor::iPhone) {
			A_target* pa_iphone = new A_target [count];
			ar	| exchange (ptr(pa,count), ptr(pa_iphone,count))
				;
		}
	}
};

int main ()
{
	using namespace pulmotor;
	
	blit_section bs;
	
	Data data (2);
	blit_object (bs, data);

	bs.dump_gathered ();

	//
	std::vector<unsigned char> buffer;
	
	{
		std::auto_ptr<basic_output_buffer> po = create_plain_output (pulmotor_native_path("ser_test.pulmotor"));
		size_t written = 0;
		po->write (&*buffer.begin (), buffer.size (), &written);
	}

	return 1;
}
