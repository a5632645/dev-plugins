#include <array>
#include <cstddef>
#include "qwqdsp/spectral/real_fft.hpp"
#include "qwqdsp/spectral/reassignment.hpp"
#include "qwqdsp/osciilor/raw_oscillor.hpp"
#include "OLEDDisplayRGB.h"
#include "qwqdsp/window/hamming.hpp"
#include "qwqdsp/window/helper.hpp"
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

int main() {
    qwqdsp::oscillor::RawOscillor osc;
    osc.Init(48000.0f);
    osc.SetFreq(5000.0f);

    std::array<float, 2048> test;
    for (auto& s : test) {
        s = osc.Sine();
    }

    qwqdsp::spectral::RealFFT fft;
    fft.Init(2048);
    std::vector<std::complex<float>> fft_cpxs;
    fft_cpxs.resize(fft.NumBins());
    auto copy = test;
    auto fft_window = copy;
    qwqdsp::window::Hamming::Window(fft_window);
    qwqdsp::window::Helper::Normalize(fft_window);
    for (size_t i = 0; i < fft.FFTSize(); ++i) {
        copy[i] = test[i] * fft_window[i];
    }
    fft.FFT(copy, fft_cpxs);
    Canvas img{1280, 720};
    for (size_t i = 0; i < fft.NumBins(); ++i) {
        float freq = i / fft.FFTSizeFloat();
        float gain = std::abs(fft_cpxs[i]);
        float db = 20.0f * std::log10f(gain + 1e-18f);
        float x = freq * img.width;
        float y = img.height * (1.0f - (db + 100) / 100.0f);
        img.g.drawVerticalLine(x, y, img.height - y);
    }
    img.SaveImage("fft.png");

    qwqdsp::spectral::Reassignment nc;
    std::vector<float> result;
    nc.Init(2048);
    nc.Process(test);

    img.g.Fill(OledColorEnum::kOledBLACK);
    for (size_t i = 0; i < nc.NumData(); ++i) {
        float freq = nc.GetFrequency(i);
        float gain = nc.GetGain(i);
        float db = 20.0f * std::log10f(gain + 1e-18f);
        float x = freq * img.width;
        float y = img.height * (1.0f - (db + 100) / 100.0f);
        img.g.drawVerticalLine(x, y, img.height - y);
    }
    img.SaveImage("reassignment.png");

    qwqdsp::spectral::ReassignmentCorrect reass2;
    reass2.Init(2048);
    reass2.Process(test);
    img.g.Fill(OledColorEnum::kOledBLACK);
    for (size_t i = 0; i < nc.NumData(); ++i) {
        float freq = reass2.GetFrequency(i);
        float gain = reass2.GetGain(i);
        float db = 20.0f * std::log10f(gain + 1e-18f);
        float x = freq * img.width;
        float y = img.height * (1.0f - (db + 100) / 100.0f);
        img.g.drawVerticalLine(x, y, img.height - y);
    }
    img.SaveImage("reass2.png");

    return 0;
}