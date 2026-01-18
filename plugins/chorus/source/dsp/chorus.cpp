#include "chorus.hpp"

namespace chorus {

template <size_t N>
static constexpr std::array<float, N> MakePanTable(size_t nvocice) {
    std::array<float, 16> out{};
    if (nvocice == 2) {
        out[0] = -1.0f;
        out[1] = 1.0f;
    }
    else {
        float interval = 2.0f / (static_cast<float>(nvocice) - 1);
        float begin = -1.0f;
        for (size_t i = 0; i < nvocice; ++i) {
            out[i] = begin + static_cast<float>(i) * interval;
        }
    }
    return out;
}

// clang-format off
static constexpr std::array<std::array<float, 16>, 16> kPanTable{
    MakePanTable<16>(2),
    MakePanTable<16>(3),
    MakePanTable<16>(4),
    MakePanTable<16>(5),
    MakePanTable<16>(6),
    MakePanTable<16>(7),
    MakePanTable<16>(8),
    MakePanTable<16>(9),
    MakePanTable<16>(10),
    MakePanTable<16>(11),
    MakePanTable<16>(12),
    MakePanTable<16>(13),
    MakePanTable<16>(14),
    MakePanTable<16>(15),
    MakePanTable<16>(16),
};
// clang-format on

void Chorus::Init(float fs) {
    for (size_t i = 0; i < 16; ++i) {
        pitch_shifter_[i].init(Pitcher::WindowMode::kMedium);
    }
}

void Chorus::Process(float* left, float* right, size_t num_samples) {
    while (num_samples--) {
        float lx = *left;
        float rx = *right;

        float ly = 0;
        float ry = 0;
        for (size_t i = 0; i < 16; ++i) {
            pitch_shifter_[i].update(lx, rx);
            ly += pitch_shifter_[i].outL;
            ry += pitch_shifter_[i].outR;
        }

        *left = ly;
        *right = ry;

        ++left;
        ++right;
    }
}

void Chorus::Update() {
    auto const& lut = kPanTable[14];
    for (size_t i = 0; i < 16; ++i) {
        float pitch_shift = detune * lut[i];
        pitch_shifter_[i].setSpeed(pitch_shifter_[i].getSpeedFromSemis(pitch_shift));
    }
}

} // namespace chorus
