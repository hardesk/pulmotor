#include <pulmotor/ser.hpp>
#include <pulmotor/stream.hpp>
#include <string>

namespace data {

struct A
{
	float	c;
	
	template<class ArchiveT>
	void blit (ArchiveT& ar, unsigned version) {
		ar | c;
	}
};

struct B
{
	A*		pa;
	A*		pz;
	
	template<class ArchiveT>
	void blit (ArchiveT& ar, unsigned version) {
		ar | pa | pz;
	}
};

} // penis

int main ()
{
	using namespace pulmotor;
	
	blit_section bs;
	
	data::B b;
	b.pa = new data::A [16];
	b.pz = 0;
	blit_object (bs, b);

	bs.dump_gathered ();

	//
	std::vector<unsigned char> buffer;
	{
		std::vector<unsigned char> buf;
		pulmotor::util::blit_to_container (b, buf, false);
	}

	{
		std::auto_ptr<basic_output_buffer> po = create_plain_output (pulmotor_native_path("pointer_test.pulmotor"));
		size_t written = 0;
		po->write (&*buffer.begin (), buffer.size (), &written);
	}

	return 1;
}
