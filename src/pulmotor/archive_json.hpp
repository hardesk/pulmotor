#ifndef PULMOTOR_ARCHIVE_JSON_HPP
#define PULMOTOR_ARCHIVE_JSON_HPP

#include "pulmotor_config.hpp"
#include "archive.hpp"

#include <rapidjson/rapidjson.h>
#include <rapidjson/writer.h>

#include <type_traits>


namespace pulmotor
{

template<class JsonWriter>
class json_output_archive : public pulmotor_archive
{
	JsonWriter& m_json;

public:
	enum { is_reading = 0, is_writing = 1 };

	json_output_archive (JsonWriter& jsonW) : m_json (jsonW)
	{
		m_json.StartArray();
	}
	~json_output_archive ()
	{
		m_json.EndArray();
	}

	void object_name(char const* name) { m_json.String(name); }

	void start_object () { m_json.StartObject(); }
	void end_object () { m_json.EndObject(); }

	void start_array () { m_json.StartArray(); }
	void end_array () { m_json.EndArray(); }

	template<int Align, class T>
	void write_basic (T data)
	{
		write_basic(data);
	}

	void write_basic (bool data) { m_json.Bool (data); }

	void write_basic (char data) { m_json.Int (data); }
	void write_basic (unsigned char data) { m_json.Int (data); }


	void write_basic (int data) { m_json.Int (data); }
	void write_basic (unsigned data) { m_json.Uint (data); }

	void write_basic (long data) { m_json.Int (data); }
	void write_basic (unsigned long data) { m_json.Uint (data); }

	void write_basic (long long data) { m_json.Int64 (data); }
	void write_basic (unsigned long long data) { m_json.Uint64 (data); }

	void write_basic (float data) { m_json.Double (data); }
	void write_basic (double data) { m_json.Double (data); }

	void write_data (void const* p, size_t count)
	{
		m_json.StartArray();
		for (unsigned char* s=(unsigned char*)p; count--; ++s)
			m_json.Int (*s);
		m_json.EndArray();

	}
};

}


#endif
