#ifndef PULMOTOR_CONTAINER_FWD_HPP_
#define PULMOTOR_CONTAINER_FWD_HPP_

#include <memory>
#include <fstream>
#include "pulmotor_config.hpp"

namespace pulmotor
{
#ifdef _WIN32
	typedef std::wstring string;
#else
	typedef std::string string;
#endif

	class PULMOTOR_ATTR_DLL basic_input_buffer;
	class PULMOTOR_ATTR_DLL basic_output_buffer;

	class PULMOTOR_ATTR_DLL basic_version;
	class PULMOTOR_ATTR_DLL basic_header;

	std::auto_ptr<basic_input_buffer> PULMOTOR_ATTR_DLL create_plain_input( pulmotor::string const& file_name );
	std::auto_ptr<basic_output_buffer> PULMOTOR_ATTR_DLL create_plain_output( pulmotor::string const& file_name );
}

#endif // PULMOTOR_CONTAINER_FWD_HPP_
