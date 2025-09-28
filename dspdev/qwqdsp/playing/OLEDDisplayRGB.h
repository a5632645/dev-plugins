#pragma once

#include <cstdint>
#include <string_view>
#include "Rectange.hpp"
#include "usf.hpp"

enum class OledColorEnum
{
    kOledBLACK = 0,
    kOledWHITE = 1,
    kOledINVERSE = 2
};

enum class OledDisplayTextAlignEnum
{
    kLeft = 0,
    kRight = 1,
    kXCenter = 2,
    kCenter = 3
};

struct OLEDRGBColor {
    uint8_t r_{}, g_{}, b_{}, a_{0xff};
    constexpr OLEDRGBColor() = default;
    constexpr OLEDRGBColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
        r_ = r;
        g_ = g;
        b_ = b;
        a_ = a;
    }
    void Inverse() {
        r_ = 255 - r_;
        g_ = 255 - g_;
        b_ = 255 - b_;
    }
};

namespace colors {

static constexpr auto white = OLEDRGBColor{0xff, 0xff, 0xff};
static constexpr auto black = OLEDRGBColor{0, 0, 0};

}

class OLEDDisplay
{
public:
    // static constexpr auto kWidth = 1280;
    // static constexpr auto kHeight = 720;
    // static constexpr auto kBufferSize = kWidth * kHeight;
    static constexpr auto kMaxPrintfString = 128;
    inline static char printBuffer[kMaxPrintfString];

    OLEDDisplay(int w, int h);

    void setColor(OledColorEnum color);
    OledColorEnum getColor();
    void Fill(OledColorEnum color);
    void setPixel(int16_t x, int16_t y);
    void setPixelColor(int16_t x, int16_t y, OledColorEnum color);
    void clearPixel(int16_t x, int16_t y);
    void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1);
    void drawRect(int16_t x, int16_t y, int16_t width, int16_t height);
    void fillRect(int16_t x, int16_t y, int16_t width, int16_t height);
    void drawCircle(int16_t x, int16_t y, int16_t radius);
    void drawCircleQuads(int16_t x0, int16_t y0, int16_t radius, uint8_t quads);
    void fillCircle(int16_t x, int16_t y, int16_t radius);
    void drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2);
    void fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2);
    void drawHorizontalLine(int16_t x, int16_t y, int16_t length);
    void drawVerticalLine(int16_t x, int16_t y, int16_t length);
    void drawProgressBar(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t progress);
    void drawFastImage(int16_t x, int16_t y, int16_t width, int16_t height, const uint8_t *image);
    void drawXbm(int16_t x, int16_t y, int16_t width, int16_t height, const uint8_t *xbm);
    void drawIco16x16(int16_t x, int16_t y, const uint8_t *ico, bool inverse = false);

    // ---------------------------------------- Text function ----------------------------------------
    uint16_t drawString(int16_t x, int16_t y, std::string_view text);
    uint16_t drawStringMaxWidth(int16_t x, int16_t y, uint16_t maxLineWidth, std::string_view text);
    uint16_t getStringWidth(std::string_view text);
    void setTextAlignment(OledDisplayTextAlignEnum textAlignment);
    OledDisplayTextAlignEnum GetTextAlignment() const { return textAlignment; }
    template <typename... Args> USF_CPP14_CONSTEXPR
    uint16_t FormatString(int16_t x, int16_t y, usf::StringView fmt, Args&&... args) {
        usf::StringSpan span = usf::format_to(usf::StringSpan(printBuffer, kMaxPrintfString), fmt, std::forward<Args>(args)...);
        return drawString(x, y, std::string_view(span.data(), span.size()));
    }
    template<class... Args> USF_CPP14_CONSTEXPR
    usf::StringSpan FormantToInternalBuffer(usf::StringView fmt, Args&&... args) {
        return usf::format_to(usf::StringSpan(printBuffer, kMaxPrintfString), fmt, std::forward<Args>(args)...);
    }

    // ---------------------------------------- Screen ----------------------------------------
    constexpr uint16_t getWidth(void) { return kWidth; }
    constexpr uint16_t getHeight(void) { return kHeight; }
    constexpr Rectange GetDrawAera(void) { return Rectange(0, 0, kWidth, kHeight); }
    int16_t GetFontHeight(void);

    // get display buffer
    OLEDRGBColor* getDisplayBuffer(void) { return buffer_; }
    void SetDisplayBuffer(OLEDRGBColor* buffer) { buffer_ = buffer; }

protected:
    OLEDRGBColor& GetPixelUncheck(int16_t x, int16_t y) {
        assert(x >= 0 && x < kWidth);
        assert(y >= 0 && y < kHeight);
        return buffer_[x + y * kWidth]; 
    }
    OLEDRGBColor* GetPixelPtrUncheck(int16_t x, int16_t y) {
        assert(x >= 0 && x < kWidth);
        assert(y >= 0 && y < kHeight);
        return &buffer_[x + y * kWidth]; 
    }
    void drawInternal(int16_t xMove, int16_t yMove, int16_t width, int16_t height, const uint8_t* data, uint32_t offset, uint16_t bytesInData);
    uint16_t drawStringInternal(int16_t xMove, int16_t yMove, std::string_view text, uint16_t textWidth, uint16_t boundWidth);

    OledDisplayTextAlignEnum textAlignment;
    OledColorEnum color;
    int kWidth;
    int kHeight;
    int kBufferSize;

    OLEDRGBColor* buffer_{};
};
