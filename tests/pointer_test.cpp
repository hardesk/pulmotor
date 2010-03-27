#include <pulmotor/ser.hpp>
#include <pulmotor/stream.hpp>
#include <string>

namespace data {

struct A
{
	float	c;

	A() : c(0) {}
	
	template<class ArchiveT>
	void serialize (ArchiveT& ar, unsigned version) {
		ar | c;
	}
};

struct B
{
	A*		pz;
	A*		pa;
	
	template<class ArchiveT>
	void serialize (ArchiveT& ar, unsigned version) {
		ar | pz | pulmotor::ptr (pa, 16);
	}
};

}

int main ()
{
	using namespace pulmotor;
	
	blit_section bs;
	
	data::B b;
	b.pa = new data::A [16];
	b.pz = 0;
	bs | b;

	bs.dump_gathered ();

	std::vector<unsigned char> buffer;
	pulmotor::util::blit_to_container (b, buffer, true);
	
	util::hexdump (&buffer[0], buffer.size ());
	util::write_file ("pointer_test.pulmotor", &buffer[0], buffer.size ());

	return 0;
}
