#include "phaser_simd.hpp"

#include <cstring>
#include <algorithm>
#include <vector>
#include <immintrin.h>  // AVX

std::unique_ptr<PhaserSIMD> createPhaserSIMD_AVX();

//==============================================================================
//  AVX phaser implementation
//  Uses 256-bit vectors. Layout: [L, R, L, R, L, R, L, R]
//  Upper 6 lanes mirror lower 2 — we process both channels with
//  redundancy for full register utilization.
//==============================================================================

class PhaserSIMD_AVX final : public PhaserSIMD {
public:
    void prepare(double /*sampleRate*/, int /*maxBlockSize*/, int maxStages) override {
        maxStages_ = maxStages;
        // Pad by 4 so _mm_loadu_ps / _mm256_broadcast_ps never reads past end
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

        // Broadcast scalars to full 256-bit
        const __m256 va    = _mm256_set1_ps(a);
        (void)feedback; (void)mix;

        for (int i = 0; i < numSamples; ++i) {
            float inL = input[i * 2 + 0];
            float inR = input[i * 2 + 1];

            float xL = inL + feedback * fbBuf_;
            float xR = inR + feedback * fbBuf_;

            for (int st = 0; st < s; ++st) {
                const auto idx = static_cast<size_t>(st) * 2;

                // Load xz1, yz1 (2 floats each)
                __m128 xz1_128 = _mm_loadu_ps(&xz1_[idx]);
                __m128 yz1_128 = _mm_loadu_ps(&yz1_[idx]);

                // Broadcast to 256: [L,R, L,R, L,R, L,R]
                __m256 vxz1 = _mm256_insertf128_ps(
                    _mm256_castps128_ps256(xz1_128), xz1_128, 1);
                __m256 vyz1 = _mm256_insertf128_ps(
                    _mm256_castps128_ps256(yz1_128), yz1_128, 1);

                __m256 vx = _mm256_set_ps(0,0, 0,0, 0,0, xR, xL);

                // y = xz1 + a * (yz1 - x)
                __m256 vy = _mm256_add_ps(vxz1,
                               _mm256_mul_ps(va, _mm256_sub_ps(vyz1, vx)));

                // Store lower 128 bits back
                _mm_storeu_ps(&xz1_[idx], _mm256_castps256_ps128(vx));
                _mm_storeu_ps(&yz1_[idx], _mm256_castps256_ps128(vy));

                // Extract L and R from result
                float tmp[4];
                _mm_storeu_ps(tmp, _mm256_castps256_ps128(vy));
                xL = tmp[0];
                xR = tmp[1];
            }

            fbBuf_ = (xL + xR) * 0.5f * feedback;

            output[i * 2 + 0] = inL + mix * (xL - inL);
            output[i * 2 + 1] = inR + mix * (xR - inR);
        }
    }

    const char* name() const noexcept override { return "AVX"; }

private:
    int maxStages_ = 0;
    std::vector<float> xz1_;
    std::vector<float> yz1_;
    float fbBuf_ = 0.0f;
};

std::unique_ptr<PhaserSIMD> createPhaserSIMD_AVX() {
    return std::make_unique<PhaserSIMD_AVX>();
}
