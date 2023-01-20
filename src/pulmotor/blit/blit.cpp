#include "blit.hpp"

namespace pulmotor
{

/*
// TODO: this should go to another file
target_traits const target_traits::current = { sizeof(void*), PULMOTOR_ENDIANESS };

target_traits const target_traits::le_lp32 = { 4, false };
target_traits const target_traits::be_lp32 = { 4, true };

target_traits const target_traits::le_lp64 = { 8, false };
target_traits const target_traits::be_lp64 = { 8, true };
*/


void set_offsets (void* data, std::pair<uintptr_t,uintptr_t> const* fixups, size_t fixup_count, size_t ptrsize, bool change_endianess)
{
    char* datap = (char*)data;
    for (size_t i=0; i<fixup_count; ++i)
    {
        switch (ptrsize)
        {
            default:
            case 2:
//				assert ("unsupported pointer size");
                break;

            case 4:
                {
                    u32* p = reinterpret_cast<u32*> (datap + fixups[i].first);
                    *p = (u32)fixups[i].second;
                    if (change_endianess)
                        swap_endian<sizeof(*p)> (p, 1);
                }
                break;
            case 8:
                {
                    u64* p = reinterpret_cast<u64*> (datap + fixups[i].first);
                    *p = (u64)fixups[i].second;
                    if (change_endianess)
                        swap_endian<sizeof(*p)> (p, 1);
                }
                break;
        }
    }
}

void fixup_pointers (void* data, uintptr_t const* fixups, size_t fixup_count)
{
    char* datap = (char*)data;
    for (size_t i=0; i<fixup_count; ++i)
    {
        uintptr_t* p = reinterpret_cast<uintptr_t*> (datap + fixups[i]);
        *p += (uintptr_t)data;
    }
}

void* fixup_pointers_impl (blit_section_info* bsi)
{
    char* data = ((char*)bsi + bsi->data_offset);
    uintptr_t* fixups = (uintptr_t*)((char*)bsi + bsi->fixup_offset);

    util::fixup_pointers (data, fixups, bsi->fixup_count);

    return data;
}

}
