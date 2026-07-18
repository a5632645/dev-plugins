#define _USE_MATH_DEFINES
#include "phaser_processor.hpp"
#include "phaser_simd.hpp"

#include <cmath>
#include <algorithm>
#include <memory>

//==============================================================================
PhaserProcessor::PhaserProcessor()  = default;
PhaserProcessor::~PhaserProcessor() = default;

//==============================================================================
void PhaserProcessor::prepare(double sampleRate, int maxBlockSize) {
    sampleRate_    = sampleRate;
    maxBlockSize_  = maxBlockSize;
    lfoPhase_      = 0.0;

    // Create best SIMD backend
    simd_ = PhaserSIMD::create();
    simd_->prepare(sampleRate, maxBlockSize, param_ranges::stagesMax);

    reset();
}

void PhaserProcessor::reset() {
    if (simd_) simd_->reset();
    lfoPhase_ = 0.0;
}

//==============================================================================
void PhaserProcessor::process(float* left, float* right, int numSamples) {
    if (simd_ == nullptr) return;

    const float rate     = rate_.load(std::memory_order_relaxed);
    const float depth    = depth_.load(std::memory_order_relaxed);
    const float feedback = feedback_.load(std::memory_order_relaxed);
    const float mix      = mix_.load(std::memory_order_relaxed);
    const int   stages   = stages_.load(std::memory_order_relaxed);

    // Build interleaved buffer for SIMD processing
    // (small stack buffer to avoid allocation for typical block sizes)
    constexpr int kStackMax = 1024;
    float* interleaved = nullptr;
    std::unique_ptr<float[]> heapBuf;

    const size_t totalFloats = static_cast<size_t>(numSamples) * 2;
    if (totalFloats <= kStackMax) {
        interleaved = (float*)alloca(totalFloats * sizeof(float));
    } else {
        heapBuf = std::make_unique<float[]>(totalFloats);
        interleaved = heapBuf.get();
    }

    // Interleave: [L0,R0, L1,R1, …]
    for (int i = 0; i < numSamples; ++i) {
        interleaved[i * 2 + 0] = left[i];
        interleaved[i * 2 + 1] = right[i];
    }

    // Process in sub-blocks (SIMD backends may have vector-length constraints)
    constexpr int kSubBlock = 64;
    const double phaseInc = rate / sampleRate_;

    for (int offset = 0; offset < numSamples; offset += kSubBlock) {
        const int n = std::min(kSubBlock, numSamples - offset);

        // Compute LFO and coefficient for each sample in this sub-block.
        // We process sample-by-sample inside the SIMD backend because
        // the coefficient changes each sample (LFO modulation).
        for (int i = 0; i < n; ++i) {
            const int idx = (offset + i) * 2;

            // Sine LFO → [-1, 1]
            const double lfo = std::sin(2.0 * M_PI * lfoPhase_);
            lfoPhase_ += phaseInc;
            if (lfoPhase_ >= 1.0) lfoPhase_ -= 1.0;

            // Map LFO → cutoff frequency → all-pass coefficient `a`
            //   t ∈ [0, 1]
            //   fc = 20 · 100^t   → [20 Hz … 2000 Hz]
            const double t   = (lfo * depth + 1.0) * 0.5;
            const double fc  = 20.0 * std::pow(100.0, t);
            const double g   = std::tan(M_PI * fc / sampleRate_);
            const float  a   = static_cast<float>((g - 1.0) / (g + 1.0));

            // Process this single stereo frame through all stages
            simd_->processBlock(interleaved + idx, interleaved + idx,
                                1, a, feedback, mix, stages);
        }
    }

    // De-interleave back
    for (int i = 0; i < numSamples; ++i) {
        left[i]  = interleaved[i * 2 + 0];
        right[i] = interleaved[i * 2 + 1];
    }
}

const char* PhaserProcessor::backendName() const noexcept {
    return simd_ ? simd_->name() : "none";
}
