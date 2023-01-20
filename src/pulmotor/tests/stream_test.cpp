#include <pulmotor/stream.hpp>

#include <algorithm>

int main()
{
    using namespace pulmotor;
    using namespace std;

    char buf [8];
    std::fill_n (buf, 8, 'x');

    // write data
    {
        std::auto_ptr<basic_output_buffer> p = create_plain_output (pulmotor_native_path("test1.pulmotor"));

        size_t written = 0;
        error_id result = p->write (buf, 8, &written);

        printf ("test1: result %ul, written: %u\n", result, (unsigned)written);
    }

    // read back
    {
        std::auto_ptr<basic_input_buffer> p = create_plain_input (pulmotor_native_path("test1.pulmotor"));
        char buf1 [8];

        size_t read = 0;
        error_id result = p->read (buf1, 8, &read);

        printf ("test1: result %d, read: %u, cmp: %d\n", result, (unsigned)read, memcmp(buf, buf1, 8));
    }


    return 0;
}
