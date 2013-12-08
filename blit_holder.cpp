#include "blit_holder.hpp"
#include "ser.hpp"

namespace pulmotor { namespace detail {
	
void load_impl (char const* fname, size_t& size, u8*& data)
{
	pulmotor::file_size_t fs = pulmotor::get_file_size (fname);
	size = (size_t)fs;
	
	if (fs)
	{			
		std::auto_ptr<pulmotor::basic_input_buffer> i = pulmotor::create_input (fname);
		
		if (i.get ())
		{
			u8* dat = new u8 [fs];
			
			std::error_code ec;
			size_t was_read = i->read (dat, (size_t)fs, ec);
			if (ec || was_read != fs) {
				delete []dat;
				return;
			}
			
			data = dat;
			
			pulmotor::blit_section_info* bsi = pulmotor::util::get_bsi (data);
			pulmotor::util::fixup (bsi);
		}
	}
}


} }