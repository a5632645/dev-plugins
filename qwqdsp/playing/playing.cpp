#include <cstddef>
#include <vector>
#include "OLEDDisplayRGB.h"
#include "qwqdsp/window/helper.hpp"
#include "stb_image_write.h"

#include "qwqdsp/window/window.hpp"
#include "qwqdsp/window/blackman.hpp"
#include "qwqdsp/window/kaiser.hpp"
#include "qwqdsp/window/taylor.hpp"
#include "qwqdsp/spectral/real_fft.hpp"

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

int main() {
    float test[64];
    float test2[64];
    qwqdsp::window::Taylor::Window(test, test2, 35.0f, 4);
    // qwqdsp::window::Taylor::Window(test, 40.0f, 8);

    qwqdsp::spectral::RealFFT fft;
    float ffts[16384];
    qwqdsp::window::Helper::ZeroPad(ffts, test);
    fft.Init(16384);
    std::vector<std::complex<float>> spectral;
    spectral.resize(fft.NumBins());
    fft.FFT(ffts, spectral);

    std::vector<float> gains;
    gains.resize(spectral.size());
    for (size_t i = 0; i < spectral.size(); ++i) {
        gains[i] = std::abs(spectral[i]) * 2.0f / fft.FFTSizeFloat();
        gains[i] = std::log10(gains[i] + 1e-18f) * 20.0f;
    }

    return 0;
}