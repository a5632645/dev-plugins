#include "OLEDDisplayRGB.h"
#include <cmath>
#include "FontRGB.hpp"

#if _OLED_TYPE == _OLED_RGB

template <class T>
    requires std::is_pointer_v<T *>
static constexpr uint8_t pgm_read_byte(T *addr)
{
    return *reinterpret_cast<uint8_t *>(addr);
}

template <class T>
    requires std::is_pointer_v<T *>
static constexpr uint8_t pgm_read_byte(const T *addr)
{
    return *reinterpret_cast<const uint8_t *>(addr);
}

using enum OledColorEnum;
using enum OledDisplayTextAlignEnum;

OLEDDisplay::OLEDDisplay(int w, int h)
{
    kWidth = w;
    kHeight = h;
    kBufferSize = w * h;
    color = kOledWHITE;
    textAlignment = kLeft;
}

void OLEDDisplay::setColor(OledColorEnum color)
{
    this->color = color;
}

OledColorEnum OLEDDisplay::getColor()
{
    return this->color;
}

void OLEDDisplay::Fill(OledColorEnum color)
{
    if (color == kOledBLACK) {
        std::fill_n(buffer_, kBufferSize, colors::black);
    }
    else if (color == kOledWHITE) {
        std::fill_n(buffer_, kBufferSize, colors::white);
    }
    else {
        for (int i = 0; i < kBufferSize; i++) {
            buffer_[i].Inverse();
        }
    }
}

void OLEDDisplay::setPixel(int16_t x, int16_t y)
{
    if (x >= 0 && x < this->getWidth() && y >= 0 && y < this->getHeight())
    {
        switch (color)
        {
        case kOledWHITE:
            GetPixelUncheck(x, y) = colors::white;
            break;
        case kOledBLACK:
            GetPixelUncheck(x, y) = colors::black;
            break;
        case kOledINVERSE:
            GetPixelUncheck(x, y).Inverse();
            break;
        }
    }
}

void OLEDDisplay::setPixelColor(int16_t x, int16_t y, OledColorEnum color)
{
    if (x >= 0 && x < this->getWidth() && y >= 0 && y < this->getHeight())
    {
        switch (color)
        {
        case kOledWHITE:
            GetPixelUncheck(x, y) = colors::white;
            break;
        case kOledBLACK:
            GetPixelUncheck(x, y) = colors::black;
            break;
        case kOledINVERSE:
            GetPixelUncheck(x, y).Inverse();
            break;
        }
    }
}

void OLEDDisplay::clearPixel(int16_t x, int16_t y)
{
    // if (x >= 0 && x < this->getWidth() && y >= 0 && y < this->getHeight())
    // {
    //     switch (color)
    //     {
    //     case kOledBLACK:
    //         buffer_[x + (y >> 3) * this->getWidth()] |= (1 << (y & 7));
    //         break;
    //     case kOledWHITE:
    //         buffer_[x + (y >> 3) * this->getWidth()] &= ~(1 << (y & 7));
    //         break;
    //     case kOledINVERSE:
    //         buffer_[x + (y >> 3) * this->getWidth()] ^= (1 << (y & 7));
    //         break;
    //     }
    // }
}

// Bresenham's algorithm - thx wikipedia and Adafruit_GFX
void OLEDDisplay::drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1)
{
    int16_t steep = abs(y1 - y0) > abs(x1 - x0);
    if (steep)
    {
        std::swap(x0, y0);
        std::swap(x1, y1);
    }

    if (x0 > x1)
    {
        std::swap(x0, x1);
        std::swap(y0, y1);
    }

    int16_t dx, dy;
    dx = x1 - x0;
    dy = abs(y1 - y0);

    int16_t err = dx / 2;
    int16_t ystep;

    if (y0 < y1)
    {
        ystep = 1;
    }
    else
    {
        ystep = -1;
    }

    for (; x0 <= x1; x0++)
    {
        if (steep)
        {
            setPixel(y0, x0);
        }
        else
        {
            setPixel(x0, y0);
        }
        err -= dy;
        if (err < 0)
        {
            y0 += ystep;
            err += dx;
        }
    }
}

void OLEDDisplay::drawRect(int16_t x, int16_t y, int16_t width, int16_t height)
{
    drawHorizontalLine(x, y, width);
    drawVerticalLine(x, y, height);
    drawVerticalLine(x + width - 1, y, height);
    drawHorizontalLine(x, y + height - 1, width);
}

void OLEDDisplay::fillRect(int16_t xMove, int16_t yMove, int16_t width, int16_t height)
{
    for (int16_t x = xMove; x < xMove + width; x++)
    {
        drawVerticalLine(x, yMove, height);
    }
}

void OLEDDisplay::drawCircle(int16_t x0, int16_t y0, int16_t radius)
{
    int16_t x = 0, y = radius;
    int16_t dp = 1 - radius;
    do
    {
        if (dp < 0)
            dp = dp + (x++) * 2 + 3;
        else
            dp = dp + (x++) * 2 - (y--) * 2 + 5;

        setPixel(x0 + x, y0 + y); // For the 8 octants
        setPixel(x0 - x, y0 + y);
        setPixel(x0 + x, y0 - y);
        setPixel(x0 - x, y0 - y);
        setPixel(x0 + y, y0 + x);
        setPixel(x0 - y, y0 + x);
        setPixel(x0 + y, y0 - x);
        setPixel(x0 - y, y0 - x);

    } while (x < y);

    setPixel(x0 + radius, y0);
    setPixel(x0, y0 + radius);
    setPixel(x0 - radius, y0);
    setPixel(x0, y0 - radius);
}

void OLEDDisplay::drawCircleQuads(int16_t x0, int16_t y0, int16_t radius, uint8_t quads)
{
    int16_t x = 0, y = radius;
    int16_t dp = 1 - radius;
    while (x < y)
    {
        if (dp < 0)
            dp = dp + (x++) * 2 + 3;
        else
            dp = dp + (x++) * 2 - (y--) * 2 + 5;
        if (quads & 0x1)
        {
            setPixel(x0 + x, y0 - y);
            setPixel(x0 + y, y0 - x);
        }
        if (quads & 0x2)
        {
            setPixel(x0 - y, y0 - x);
            setPixel(x0 - x, y0 - y);
        }
        if (quads & 0x4)
        {
            setPixel(x0 - y, y0 + x);
            setPixel(x0 - x, y0 + y);
        }
        if (quads & 0x8)
        {
            setPixel(x0 + x, y0 + y);
            setPixel(x0 + y, y0 + x);
        }
    }
    if (quads & 0x1 && quads & 0x8)
    {
        setPixel(x0 + radius, y0);
    }
    if (quads & 0x4 && quads & 0x8)
    {
        setPixel(x0, y0 + radius);
    }
    if (quads & 0x2 && quads & 0x4)
    {
        setPixel(x0 - radius, y0);
    }
    if (quads & 0x1 && quads & 0x2)
    {
        setPixel(x0, y0 - radius);
    }
}

void OLEDDisplay::fillCircle(int16_t x0, int16_t y0, int16_t radius)
{
    int16_t x = 0, y = radius;
    int16_t dp = 1 - radius;
    do
    {
        if (dp < 0)
            dp = dp + (x++) * 2 + 3;
        else
            dp = dp + (x++) * 2 - (y--) * 2 + 5;

        drawHorizontalLine(x0 - x, y0 - y, 2 * x);
        drawHorizontalLine(x0 - x, y0 + y, 2 * x);
        drawHorizontalLine(x0 - y, y0 - x, 2 * y);
        drawHorizontalLine(x0 - y, y0 + x, 2 * y);

    } while (x < y);
    drawHorizontalLine(x0 - radius, y0, 2 * radius);
}

void OLEDDisplay::drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                            int16_t x2, int16_t y2)
{
    drawLine(x0, y0, x1, y1);
    drawLine(x1, y1, x2, y2);
    drawLine(x2, y2, x0, y0);
}

void OLEDDisplay::fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                            int16_t x2, int16_t y2)
{
    int16_t a, b, y, last;

    if (y0 > y1)
    {
        std::swap(y0, y1);
        std::swap(x0, x1);
    }
    if (y1 > y2)
    {
        std::swap(y2, y1);
        std::swap(x2, x1);
    }
    if (y0 > y1)
    {
        std::swap(y0, y1);
        std::swap(x0, x1);
    }

    if (y0 == y2)
    {
        a = b = x0;
        if (x1 < a)
        {
            a = x1;
        }
        else if (x1 > b)
        {
            b = x1;
        }
        if (x2 < a)
        {
            a = x2;
        }
        else if (x2 > b)
        {
            b = x2;
        }
        drawHorizontalLine(a, y0, b - a + 1);
        return;
    }

    int16_t
        dx01 = x1 - x0,
        dy01 = y1 - y0,
        dx02 = x2 - x0,
        dy02 = y2 - y0,
        dx12 = x2 - x1,
        dy12 = y2 - y1;
    int32_t
        sa = 0,
        sb = 0;

    if (y1 == y2)
    {
        last = y1; // Include y1 scanline
    }
    else
    {
        last = y1 - 1; // Skip it
    }

    for (y = y0; y <= last; y++)
    {
        a = x0 + sa / dy01;
        b = x0 + sb / dy02;
        sa += dx01;
        sb += dx02;

        if (a > b)
        {
            std::swap(a, b);
        }
        drawHorizontalLine(a, y, b - a + 1);
    }

    sa = dx12 * (y - y1);
    sb = dx02 * (y - y0);
    for (; y <= y2; y++)
    {
        a = x1 + sa / dy12;
        b = x0 + sb / dy02;
        sa += dx12;
        sb += dx02;

        if (a > b)
        {
            std::swap(a, b);
        }
        drawHorizontalLine(a, y, b - a + 1);
    }
}

void OLEDDisplay::drawHorizontalLine(int16_t x, int16_t y, int16_t length)
{
    if (y < 0 || y >= this->getHeight())
    {
        return;
    }

    if (x < 0)
    {
        length += x;
        x = 0;
    }

    if ((x + length) > this->getWidth())
    {
        length = (this->getWidth() - x);
    }

    if (length <= 0)
    {
        return;
    }

    auto* ptr = GetPixelPtrUncheck(x, y);
    switch (color)
    {
    case kOledWHITE:
        while (length--)
        {
            *ptr = colors::white;
            ++ptr;
        };
        break;
    case kOledBLACK:
        while (length--)
        {
            *ptr = colors::black;
            ++ptr;
        };
        break;
    case kOledINVERSE:
        while (length--)
        {
            ptr->Inverse();
            ++ptr;
        };
        break;
    }
}

void OLEDDisplay::drawVerticalLine(int16_t x, int16_t y, int16_t length)
{
    if (x < 0 || x >= this->getWidth())
        return;

    if (y < 0)
    {
        length += y;
        y = 0;
    }

    if ((y + length) > this->getHeight())
    {
        length = (this->getHeight() - y);
    }

    if (length <= 0)
        return;

    switch(color) {
    case kOledBLACK:
        for (int16_t i = 0; i < length; ++i) {
            GetPixelUncheck(x, y + i) = colors::black;
        }
        break;
    case kOledWHITE:
        for (int16_t i = 0; i < length; ++i) {
            GetPixelUncheck(x, y + i) = colors::white;
        }
        break;
    case kOledINVERSE:
        for (int16_t i = 0; i < length; ++i) {
            GetPixelUncheck(x, y + i).Inverse();
        }
        break;
    }
}

void OLEDDisplay::drawProgressBar(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t progress)
{
    uint16_t radius = height / 2;
    uint16_t xRadius = x + radius;
    uint16_t yRadius = y + radius;
    uint16_t doubleRadius = 2 * radius;
    uint16_t innerRadius = radius - 2;

    setColor(kOledWHITE);
    drawCircleQuads(xRadius, yRadius, radius, 0b00000110);
    drawHorizontalLine(xRadius, y, width - doubleRadius + 1);
    drawHorizontalLine(xRadius, y + height, width - doubleRadius + 1);
    drawCircleQuads(x + width - radius, yRadius, radius, 0b00001001);

    uint16_t maxProgressWidth = (width - doubleRadius + 1) * progress / 100;

    fillCircle(xRadius, yRadius, innerRadius);
    fillRect(xRadius + 1, y + 2, maxProgressWidth, height - 3);
    fillCircle(xRadius + maxProgressWidth, yRadius, innerRadius);
}

void OLEDDisplay::drawFastImage(int16_t xMove, int16_t yMove, int16_t width, int16_t height, const uint8_t *image)
{
    drawInternal(xMove, yMove, width, height, image, 0, 0);
}

void OLEDDisplay::drawXbm(int16_t xMove, int16_t yMove, int16_t width, int16_t height, const uint8_t *xbm)
{
    int16_t widthInXbm = (width + 7) / 8;
    uint8_t data = 0;

    for (int16_t y = 0; y < height; y++)
    {
        for (int16_t x = 0; x < width; x++)
        {
            if (x & 7)
            {
                data >>= 1; // Move a bit
            }
            else
            { // Read new data every 8 bit
                data = pgm_read_byte(xbm + (x / 8) + y * widthInXbm);
            }
            // if there is a bit draw it
            if (data & 0x01)
            {
                setPixel(xMove + x, yMove + y);
            }
        }
    }
}

void OLEDDisplay::drawIco16x16(int16_t xMove, int16_t yMove, const uint8_t *ico, bool inverse)
{
    uint16_t data;

    for (int16_t y = 0; y < 16; y++)
    {
        data = pgm_read_byte(ico + (y << 1)) + (pgm_read_byte(ico + (y << 1) + 1) << 8);
        for (int16_t x = 0; x < 16; x++)
        {
            if ((data & 0x01) ^ inverse)
            {
                setPixelColor(xMove + x, yMove + y, kOledWHITE);
            }
            else
            {
                setPixelColor(xMove + x, yMove + y, kOledBLACK);
            }
            data >>= 1; // Move a bit
        }
    }
}

struct UTF8ToUnicode {
    std::string_view text_;
    uint32_t pos_{};
    // Define a static constexpr variable npos with a value of uint32_t(-1)
    static constexpr uint32_t npos = -1;

    UTF8ToUnicode(std::string_view t) : text_(t) {}
    uint32_t Next() {
        if (pos_ >= text_.length()) {
            return npos; // Return npos if there are no more characters
        }

        uint32_t code = 0;
        auto c = static_cast<unsigned char>(text_[pos_]);

        if ((c & 0x80) == 0) {
            // 1-byte UTF-8 character (ASCII)
            code = c;
            pos_ += 1;
        } else if ((c & 0xE0) == 0xC0) {
            // 2-byte UTF-8 character
            code = ((c & 0x1F) << 6) | (static_cast<unsigned char>(text_[pos_ + 1]) & 0x3F);
            pos_ += 2;
        } else if ((c & 0xF0) == 0xE0) {
            // 3-byte UTF-8 character
            code = ((c & 0x0F) << 12) | ((static_cast<unsigned char>(text_[pos_ + 1]) & 0x3F) << 6) | (static_cast<unsigned char>(text_[pos_ + 2]) & 0x3F);
            pos_ += 3;
        } else if ((c & 0xF8) == 0xF0) {
            // 4-byte UTF-8 character
            code = ((c & 0x07) << 18) | ((static_cast<unsigned char>(text_[pos_ + 1]) & 0x3F) << 12) | ((static_cast<unsigned char>(text_[pos_ + 2]) & 0x3F) << 6) | (static_cast<unsigned char>(text_[pos_ + 3]) & 0x3F);
            pos_ += 4;
        } else {
            // Invalid UTF-8 start byte
            pos_ += 1;
            return npos;
        }

        return code;
    }
};

uint16_t OLEDDisplay::drawStringInternal(int16_t xMove, int16_t yMove, std::string_view text, uint16_t textWidth, uint16_t boundWidth)
{
    uint8_t textHeight = font_header_rgb.height;
    uint16_t cursorX = 0;
    uint16_t cursorY = 0;
    uint16_t charCount = 0;

    switch (textAlignment)
    {
    case kCenter:
        yMove -= textHeight >> 1;
    // Fallthrough
    case kXCenter:
        xMove += (boundWidth - textWidth) / 2;
        break;
    case kRight:
        xMove -= textWidth;
        break;
    case kLeft:
        break;
    }

    // Don't draw anything if it is not on the screen.
    if (xMove + textWidth < 0 || xMove >= this->getWidth())
    {
        return 0;
    }
    if (yMove + textHeight < 0 || yMove >= this->getHeight())
    {
        return 0;
    }
    
    UTF8ToUnicode convert{text};
    for (auto code = convert.Next(); code != UTF8ToUnicode::npos; code = convert.Next())
    {
        int16_t xPos = xMove + cursorX;
        int16_t yPos = yMove + cursorY;
        if (xPos > this->getWidth())
            break; // no need to continue
        charCount++;

        uint32_t fontIndex = GetFontIndexIndexSimpler(code);
        if (fontIndex != kInvalidFontIndexIndex) {
            auto& fontIndexInfo = font_indexs_rgb[fontIndex];
            drawInternal(xPos, yPos,
                fontIndexInfo.width, textHeight, 
                font_data_rgb, fontIndexInfo.offset, fontIndexInfo.size);
            cursorX += fontIndexInfo.width;
        }
    }

    return charCount;
}

uint16_t OLEDDisplay::drawString(int16_t xMove, int16_t yMove, std::string_view strUser)
{
    uint16_t lineHeight = font_header_rgb.height;

    uint16_t yOffset = 0;
    // If the string should be centered vertically too
    // we need to now how heigh the string is.
    if (textAlignment == kCenter)
    {
        uint16_t lb = 0;
        // Find number of linebreaks in text
        for (uint16_t i = 0; strUser[i] != 0; i++)
        {
            lb += (strUser[i] == '\n');
        }
        // Calculate center
        yOffset = (lb * lineHeight) / 2;
    }

    uint16_t charDrawn = 0;
    uint16_t line = 0;

    for (;;)
    {
        auto spliteIdx = strUser.find('\n');
        std::string_view toDraw = strUser;
        if (spliteIdx != std::string_view::npos)
        {
            toDraw = strUser.substr(0, spliteIdx - 1);
            strUser = strUser.substr(spliteIdx + 1);
        }

        auto strWidth = getStringWidth(toDraw);
        charDrawn += drawStringInternal(xMove, yMove - yOffset + (line++) * lineHeight, toDraw, strWidth, getWidth() - xMove);

        if (spliteIdx == std::string_view::npos)
        {
            break;
        }
    }

    return charDrawn;
}

uint16_t OLEDDisplay::drawStringMaxWidth(int16_t xMove, int16_t yMove, uint16_t maxLineWidth, std::string_view strUser)
{
    uint16_t lineHeight = font_header_rgb.height;

    uint16_t length = static_cast<uint16_t>(strUser.length());
    uint16_t lastDrawnPos = 0;
    uint16_t lineNumber = 0;
    uint16_t strWidth = 0;

    uint16_t preferredBreakpoint = 0;
    uint16_t widthAtBreakpoint = 0;
    uint16_t firstLineChars = 0;
    uint16_t drawStringResult = 1; // later tested for 0 == error, so initialize to 1

    UTF8ToUnicode utf8ToUnicode(strUser);
    int i = 0;
    for (auto code = utf8ToUnicode.Next(); code != utf8ToUnicode.npos; code = utf8ToUnicode.Next())
    {
        uint32_t fontIndexIndex = GetFontIndexIndexSimpler(code);
        if (fontIndexIndex == kInvalidFontIndexIndex) {
            continue;
        }
        strWidth += font_indexs_rgb[fontIndexIndex].width;

        // Always try to break on a space, dash or slash
        if (code == ' ' || code == '-' || code == '/')
        {
            preferredBreakpoint = i + 1;
            widthAtBreakpoint = strWidth;
        }

        if (strWidth >= maxLineWidth)
        {
            if (preferredBreakpoint == 0)
            {
                preferredBreakpoint = i;
                widthAtBreakpoint = strWidth;
            }
            drawStringResult = drawStringInternal(xMove, yMove + (lineNumber++) * lineHeight,
                strUser.substr(lastDrawnPos, preferredBreakpoint - lastDrawnPos),
                widthAtBreakpoint, maxLineWidth);
            if (firstLineChars == 0)
                firstLineChars = preferredBreakpoint;
            lastDrawnPos = preferredBreakpoint;
            // It is possible that we did not draw all letters to i so we need
            // to account for the width of the chars from `i - preferredBreakpoint`
            // by calculating the width we did not draw yet.
            strWidth = strWidth - widthAtBreakpoint;
            preferredBreakpoint = 0;
            if (drawStringResult == 0) // we are past the display already?
                break;
        }
        ++i;
    }

    // Draw last part if needed
    if (drawStringResult != 0 && lastDrawnPos < length)
    {
        auto t = strUser.substr(lastDrawnPos, length - lastDrawnPos);
        auto strWidth = getStringWidth(t);
        drawStringResult = drawStringInternal(xMove, yMove + (lineNumber++) * lineHeight, t, strWidth, maxLineWidth);
    }

    if (drawStringResult == 0 || (yMove + lineNumber * lineHeight) >= this->getHeight()) // text did not fit on screen
        return firstLineChars;
    return 0; // everything was drawn
}

uint16_t OLEDDisplay::getStringWidth(std::string_view text)
{
    uint16_t stringWidth = 0;
    uint16_t maxWidth = 0;

    UTF8ToUnicode utf8ToUnicode(text);
    for (auto code = utf8ToUnicode.Next();
        code != utf8ToUnicode.npos;
        code = utf8ToUnicode.Next())
    {
        if (code == 10)
        {
            maxWidth = std::max(maxWidth, stringWidth);
            stringWidth = 0;
        }
        else {
            uint32_t fontIndexIndex = GetFontIndexIndexSimpler(code);
            if (fontIndexIndex != kInvalidFontIndexIndex) {
                stringWidth += font_indexs_rgb[fontIndexIndex].width;
            }
        }
    }

    return std::max(maxWidth, stringWidth);
}

void OLEDDisplay::setTextAlignment(OledDisplayTextAlignEnum textAlignment)
{
    this->textAlignment = textAlignment;
}

int16_t OLEDDisplay::GetFontHeight(void) {
    return font_header_rgb.height;
}

void OLEDDisplay::drawInternal(int16_t xMove, int16_t yMove, int16_t width, int16_t height, const uint8_t* data, uint32_t offset, uint16_t bytesInData)
{
    if (width <= 0 || height <= 0)
    return;
    if (yMove + height < 0 || yMove >= this->getHeight())
    return;
    if (xMove + width < 0 || xMove >= this->getWidth())
    return;
    
    uint8_t rasterWidth = 1 + ((width - 1) >> 3); // fast ceil(height / 8.0)
    int16_t x = xMove;
    for (uint16_t i = 0; i < bytesInData; ++i) {
        if (i % rasterWidth == 0) {
            x = xMove;
            ++yMove;
        }
        uint8_t byte = data[offset + i];
        for (uint8_t j = 0; j < 8; ++j) {
            if (byte & 0x80) {
                setPixel(x, yMove);
            }
            byte <<= 1;
            ++x;
        }
    }
}

#endif
