#pragma once
#include <memory>
#include <string_view>

#include "pluginshared/simd/inst.hpp"

namespace sttr {

/** All parameters in one struct — set atomically via SetParameters(). */
struct SttrParam {
    float mix{0.5f};
    float hopMs{16.0f};
    float dryDelay{0.0f};
    float stretch{1.0f};   // ratio = 2^(formant/12), formant in semitones
    int windowMul{2};        // grain length = windowMul * hop, integer [1, 4]
    float windowBeta{8.0f};  // Kaiser window beta
};

/** SIMD-dispatched STTR engine interface. */
class Idsp {
public:
    virtual ~Idsp() = default;

    /** Allocate stereo delay buffer and reset state. */
    virtual void Prepare(float sampleRate) = 0;

    /** Clear delay buffer and reset read/write positions. */
    virtual void Reset() = 0;

    /** Set all parameters atomically. */
    virtual void SetParameters(SttrParam const& params) = 0;

    /** Process one stereo audio block in-place. */
    virtual void ProcessBlock(float* left, float* right, int numSamples) = 0;

    /** Human-readable instruction-set name. */
    virtual std::string_view InstName() = 0;
};

using DspHandle = std::unique_ptr<Idsp>;

template <simd::Inst inst>
std::unique_ptr<Idsp> CreateDspImpl();

std::unique_ptr<Idsp> CreateDsp();

} // namespace sttr
