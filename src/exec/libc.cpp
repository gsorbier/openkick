#include <exec/libc.hpp>

// size_t strlen(const char *string) {
//     size_t length = 0;
//     while(*string++)
//         ++length;
//     return length;
// }

// void *memset32(void *s, int c, size_t n) {
//     uint32_t *p = static_cast<uint32_t *>(s);
//     uint32_t v = c * 0x01010101;
//     while (n-=4 >= 4)
//         *p++ = v;
//     return p;
// }

// void *memset(void *s, int c, size_t n) {
//     uint8_t *p = static_cast<uint8_t *>(s);
//     uint8_t v = c;

//     // fill with bytes until we reach a longword boundary
//     while (n-- >= 1 && reinterpret_cast<address_t>(p) & 3)
//         *p++ = v;

//     // use longword-copying memset32
//     p = static_cast<uint8_t *>(memset32(p, c, n));
//     n &= 3;

//     // now do any leftovers
//     while (n-- >= 1)
//         *p++ = v;
//     return p;
// }

#pragma GCC push_options

// \todo write optimised version
void *memset(void *s, int c, size_t n) {
    unsigned char *us = static_cast<unsigned char *>(s);
    unsigned char uc = c;
    while (n-- != 0)
        *us++ = uc;
    return s;
}

// \todo write optimised version
void *bzero(void *s, size_t n) {
    unsigned char *us = static_cast<unsigned char *>(s);
    while (n-- != 0)
        *us++ = 0;
    return s;
}

/* FIXME: this algorithm is incomplete and untested

uint32_t __udivsi3(uint32_t left, uint32_t right) {
    if(right < 65536) {
        uint16_t
            right16 = static_cast<uint16_t>(right),
            left_top = static_cast<uint16_t>(left >> 16),
            left_bottom = static_cast<uint16_t>(left);
        // FIXME
    } else {
        uint32_t orig_right = right;
        while(right >= 65536) {
            left >>= 1;
            right >>= 1;
        }
        uint16_t
            left16 = static_cast<uint16_t>(left),
            right16 = static_cast<uint16_t>(right),
            result = left16 / right16;
        // FIXME: adjust
        return result;
    }
    return 0;
};

*/
