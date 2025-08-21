#pragma once
#include <cstdint>
#include <numeric>
#include <algorithm>

#define FONT_STYLE_BLOD             0x0001 /* bit0 1~Blod */
#define FONT_STYLE_ITALIC           0x0002 /* bit1 1~Italic */
#define FONT_STYLE_GRAYBITS         0x000C /* bit3~2 GrayBits 0~3 -> 1,2,4,8 */
#define FONT_STYLE_ROTATE           0x0030 /* bit5~4 Rotate 0~3 -> 0,90,180,270 */
#define FONT_STYLE_FLIPX            0x0040 /* bit6 1~FlipX */
#define FONT_STYLE_FLIPY            0x0080 /* bit7 1~FlipY */
#define FONT_STYLE_MSB_FIRST        0x0100 /* bit8 0~LSBFirst,1~MSBFirst */
#define FONT_STYLE_HIGH_POLARITY    0x0200 /* bit9 0~LowPolarity,1~HighPolarity */
#define FONT_STYLE_LINE_ROUND       0x0400 /* bit10 0~ByteRound,1~LineRound */
#define FONT_STYLE_SCANX            0x1000 /* bit12 0~Left to Right,1~Right to Left */
#define FONT_STYLE_SCANY            0x2000 /* bit 13 0~Top to Bottom,1~Bottom to Top */
#define FONT_STYLE_SCANXY           0x4000 /* bit14 0~Horizontal then Vertical,1~Vertical then Horizontal */

#define FONT_GRAYBITS_1     0x0000; /* bit3~2 GrayBits 0~3 -> 1,2,4,8 */
#define FONT_GRAYBITS_2     0x0004;
#define FONT_GRAYBITS_4     0x0008;
#define FONT_GRAYBITS_8     0x000C;

#define FONT_ROTATE_0       0x0000 /* bit5~4 Rotate 0~3 -> 0,90,180,270 */
#define FONT_ROTATE_90      0x0010
#define FONT_ROTATE_180     0x0020
#define FONT_ROTATE_270     0x0030

typedef struct _font_header
{
    uint8_t magic[4]; /* 'F', 'N', 'T', Ver */
    uint32_t style; /* the font style */
    uint16_t height; /* the font height */
    uint16_t codepage; /* 936 GB2312, 1200 Unicode */
    int8_t padding[4]; /* left, top, right, bottom padding */

    uint16_t total_sections; /* total sections */
    uint16_t total_chars; /* total characters */
    uint32_t total_size; /* file total size or data total size */
} font_header_t;

typedef struct _font_section
{
    uint16_t first; /* first character */
    uint16_t last; /* last character */
    uint32_t offset; /* the first font_index offset */
} font_section_t;

typedef struct _font_index
{
    uint16_t width; /* the width of the character */
    uint16_t size; /* the bitmap data size */
    uint32_t offset; /* the font bitmap data offset */
} font_index_t;

extern const font_header_t font_header_rgb;
extern const font_section_t font_sections_rgb[64];
extern const font_index_t font_indexs_rgb[];
extern const uint8_t font_data_rgb[];

static constexpr uint32_t kInvalidFontIndexIndex = std::numeric_limits<uint32_t>::max();

inline static constexpr uint32_t GetFontIndexIndex(uint16_t code) {
    constexpr auto size = sizeof(font_sections_rgb) / sizeof(font_sections_rgb[0]);
    auto it = std::lower_bound(font_sections_rgb, font_sections_rgb + size, code,
        [](const font_section_t& sec, uint16_t code) {
            return sec.last < code;
        });
    if (it != font_sections_rgb + size) {
        if (code >= it->first && code <= it->last)
            return code - it->first + it->offset;
    }
    return kInvalidFontIndexIndex;
}

inline static constexpr uint32_t GetFontIndexIndexSimpler(uint16_t code) {
    // if (code >= font_sections_rgb[0].first && code <= font_sections_rgb[0].last) {
    //     return code - font_sections_rgb[0].first + font_sections_rgb[0].offset;
    // }
    // return kInvalidFontIndexIndex;
    return GetFontIndexIndex(code);
}
