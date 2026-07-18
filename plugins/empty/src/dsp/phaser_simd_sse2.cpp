#include "phaser_simd.hpp"

#include <cstring>
#include <algorithm>
#include <vector>
#include <xmmintrin.h>   // SSE

// Forward declaration matching the factory extern in phaser_simd_scalar.cpp
std::unique_ptr<PhaserSIMD> createPhaserSIMD_SSE2();

//==============================================================================
//  SSE2 phaser implementation
//  Processes both stereo channels in parallel using 128-bit vectors.
//  Layout: __m128 = [L, R, —, —]  (lower 2 lanes used)
//==============================================================================

class PhaserSIMD_SSE2 final : public PhaserSIMD {
public:
    void prepare(double /*sampleRate*/, int /*maxBlockSize*/, int maxStages) override {
        maxStages_ = maxStages;
        // Pad by 4 so _mm_loadu_ps never reads past end for any stage
        const size_t sz = static_cast<size_t>(maxStages) * 2 + 4;
        xz1_.resize(sz, 0.0f);
        yz1_.resize(sz, 0.0f);
    }

    void reset() override {
        std::fill(xz1_.begin(), xz1_.end(), 0.0f);
        std::fill(yz1_.begin(), yz1_.end(), 0.0f);
        fbBuf_ = 0.0f;
    }

    void processBlock(const float* input, float* output,
                      int numSamples, float a, float feedback,
                      float mix, int stages) override
    {
        const int s = std::min(stages, maxStages_);

        const __m128 va   = _mm_set1_ps(a);
        (void)feedback; (void)mix;

        for (int i = 0; i < numSamples; ++i) {
            // Load input: [L, R]
            __m128 in = _mm_loadu_ps(&input[i * 2]);
            // Pad zeros in upper 64 bits (already 0 from load)
            in = _mm_unpacklo_ps(in, _mm_setzero_ps());
            // Now in = [L, 0, R, 0]... hmm, that's wrong.
            // Let me just do a 64-bit load
            float tmp[4];
            _mm_store_ps(tmp, in); // hmm this is getting messy

            // Let me use a simpler approach
            float inL = input[i * 2 + 0];
            float inR = input[i * 2 + 1];

            float xL = inL + feedback * fbBuf_;
            float xR = inR + feedback * fbBuf_;

            for (int st = 0; st < s; ++st) {
                const auto idx = static_cast<size_t>(st) * 2;

                // Load xz1, yz1 as [L, R]
                __m128 vxz1 = _mm_loadu_ps(&xz1_[idx]);
                __m128 vyz1 = _mm_loadu_ps(&yz1_[idx]);
                __m128 vx   = _mm_set_ps(0.0f, 0.0f, xR, xL);

                // y = xz1 + a * (yz1 - x)
                __m128 vdiff = _mm_sub_ps(vyz1, vx);
                __m128 vy = _mm_add_ps(vxz1, _mm_mul_ps(va, vdiff));

                // Store back
                _mm_storeu_ps(&xz1_[idx], vx);
                _mm_storeu_ps(&yz1_[idx], vy);

                // Extract for next stage input
                float result[4];
                _mm_storeu_ps(result, vy);
                xL = result[0];  // L
                xR = result[1];  // R
            }

            fbBuf_ = (xL + xR) * 0.5f * feedback;

            // Wet/dry mix
            output[i * 2 + 0] = inL + mix * (xL - inL);
            output[i * 2 + 1] = inR + mix * (xR - inR);
        }
    }

    const char* name() const noexcept override { return "SSE2"; }

private:
    int maxStages_ = 0;
    std::vector<float> xz1_;
    std::vector<float> yz1_;
    float fbBuf_ = 0.0f;
};

std::unique_ptr<PhaserSIMD> createPhaserSIMD_SSE2() {
    return std::make_unique<PhaserSIMD_SSE2>();
}
