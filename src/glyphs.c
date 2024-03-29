#include "glyphs.h"

unsigned int const C_icon_app_colors[] = {
  0x00000000,
  0x00ffffff,
};

unsigned char const C_icon_app_bitmap[] = {
    0xe0, 0x01, 0xee, 0xc1, 0xf3, 0x78, 0x7d, 0xde, 0xde, 0x23, 0x78, 0x63, 0x6f, 0xec, 0x41, 0xbc,
    0xb7, 0xe7, 0xeb, 0xf1, 0x3c, 0x78, 0x07, 0x78, 0x00, };

#ifdef OS_IO_SEPROXYHAL
#include "os_io_seproxyhal.h"
const bagl_icon_details_t C_icon_app = { GLYPH_icon_app_WIDTH, GLYPH_icon_app_HEIGHT, 1, C_icon_app_colors, C_icon_app_bitmap };
#endif // OS_IO_SEPROXYHAL

#include "glyphs.h"

unsigned int const C_icon_back_colors[]
    = {
        0x00000000,
        0x00ffffff,
    };

unsigned char const C_icon_back_bitmap[] = {
    0xe0, 0x01, 0xfe, 0xc1, 0xfd, 0x38, 0x7f, 0x06, 0xdf, 0x81, 0xff, 0xc4, 0x7f, 0xf3, 0xff, 0xbc,
    0x1f, 0xe7, 0xe7, 0xf1, 0x3f, 0xf8, 0x07, 0x78, 0x00,};

#ifdef OS_IO_SEPROXYHAL
#include "os_io_seproxyhal.h"
const bagl_icon_details_t C_icon_back = { GLYPH_icon_back_WIDTH, GLYPH_icon_back_HEIGHT, 1, C_icon_back_colors, C_icon_back_bitmap };
#endif // OS_IO_SEPROXYHAL

#include "glyphs.h"

unsigned int const C_icon_dashboard_colors[]
    = {
        0x00000000,
        0x00ffffff,
    };

unsigned char const C_icon_dashboard_bitmap[] = {
    0xe0, 0x01, 0xfe, 0xc1, 0xff, 0x38, 0x70, 0x06, 0xd8, 0x79, 0x7e, 0x9e, 0x9f, 0xe7, 0xe7, 0xb9,
    0x01, 0xe6, 0xc0, 0xf1, 0x3f, 0xf8, 0x07, 0x78, 0x00,};

#ifdef OS_IO_SEPROXYHAL
#include "os_io_seproxyhal.h"
const bagl_icon_details_t C_icon_dashboard = { GLYPH_icon_dashboard_WIDTH, GLYPH_icon_dashboard_HEIGHT, 1, C_icon_dashboard_colors, C_icon_dashboard_bitmap };
#endif // OS_IO_SEPROXYHAL
