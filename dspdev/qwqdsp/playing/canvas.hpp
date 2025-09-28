#pragma once
#include <vector>
#include "OLEDDisplayRGB.h"
#include "stb_image_write.h"

struct Canvas {
    int width;
    int height;
    static constexpr int bpp = 4;

    Canvas(int width, int height)
        : width{width}
        , height{height}
        , pixels_(width * height)
        , g(width, height)
    {
        g.SetDisplayBuffer(pixels_.data());
    }

    const auto& GetPixels() const { return pixels_; }

    void SaveImage(std::string_view path) {
        stbi_write_png(path.data(), width, height, 4,
                   pixels_.data(), width * bpp);
    }

    OLEDDisplay g;
private:
    std::vector<OLEDRGBColor> pixels_;
};