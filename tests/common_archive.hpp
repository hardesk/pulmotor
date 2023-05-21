#ifndef PULMOTOR_TESTS_COMMON_ARCHIVE_HPP_
#define PULMOTOR_TESTS_COMMON_ARCHIVE_HPP_

#include <pulmotor/yaml_archive.hpp>
#include <pulmotor/stream.hpp>
#include <pulmotor/binary_archive.hpp>

class BinaryArchiveTest
{
    pulmotor::sink_vector m_sink;

public:
    pulmotor::archive_sink o;

    BinaryArchiveTest()
    :   o(m_sink)
    {}

    pulmotor::archive_memory make_reader() {
        return pulmotor::archive_memory(m_sink.data());
    }

    std::vector<char> const& data() { return m_sink.data(); }
};

class YamlArchiveTest
{
    std::stringstream m_os;
    pulmotor::yaml::writer1 m_writer;

    std::string content;
    pulmotor::yaml_document y;

public:
    pulmotor::archive_write_yaml o;

    YamlArchiveTest()
    :   m_writer(m_os), o(m_writer)
    {}

    ~YamlArchiveTest() { }

    pulmotor::archive_read_yaml make_reader() {
        content = m_os.str();
        y.parse( c4::to_substr(content), "<generated>");
        return pulmotor::archive_read_yaml(y);
    }

    std::string data() { return m_os.str(); }
};

class YamlKeyedArchiveTest
{
    std::stringstream m_os;
    pulmotor::yaml::writer1 m_writer;

    std::string content;
    pulmotor::yaml_document y;

    pulmotor::diagnostics m_diag;

public:
    pulmotor::archive_write_yaml_keyed o;

    YamlKeyedArchiveTest()
    :   m_writer(m_os), m_diag(std::cout), o(m_writer)
    {}

    ~YamlKeyedArchiveTest() { }

    pulmotor::archive_read_keyed_yaml make_reader() {
        content = m_os.str();
        y.parse( c4::to_substr(content), "<generated>");
        return pulmotor::archive_read_keyed_yaml(y, &m_diag);
    }

    std::string data() { return m_os.str(); }
};

#endif // PULMOTOR_TESTS_COMMON_ARCHIVE_HPP_
