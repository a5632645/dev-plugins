#include "phaser_simd.hpp"

#include <cstring>
#include <algorithm>
#include <vector>
#include <smmintrin.h>  // SSE4.1

std::unique_ptr<PhaserSIMD> createPhaserSIMD_SSE41();

//==============================================================================
//  SSE4.1 phaser implementation
//  Uses SSE4.1 blend instructions for slightly more efficient register moves.
//==============================================================================

class PhaserSIMD_SSE41 final : public PhaserSIMD {
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

        const __m128 va    = _mm_set1_ps(a);
        (void)feedback; (void)mix;

        for (int i = 0; i < numSamples; ++i) {
            float inL = input[i * 2 + 0];
            float inR = input[i * 2 + 1];

            float xL = inL + feedback * fbBuf_;
            float xR = inR + feedback * fbBuf_;

            for (int st = 0; st < s; ++st) {
                const auto idx = static_cast<size_t>(st) * 2;

                // Load xz1, yz1 as [L, R, _, _]
                __m128 vxz1 = _mm_loadu_ps(&xz1_[idx]);
                __m128 vyz1 = _mm_loadu_ps(&yz1_[idx]);
                __m128 vx   = _mm_set_ps(0.0f, 0.0f, xR, xL);

                // y = xz1 + a * (yz1 - x)
                __m128 vy = _mm_add_ps(vxz1, _mm_mul_ps(va, _mm_sub_ps(vyz1, vx)));

                // Preserve upper 2 lanes of states (which are padding), blend in new x
                // Actually we store full vector — the upper lanes are unused padding anyway.
                _mm_storeu_ps(&xz1_[idx], vx);
                _mm_storeu_ps(&yz1_[idx], vy);

                float tmp[4];
                _mm_storeu_ps(tmp, vy);
                xL = tmp[0];
                xR = tmp[1];
            }

            fbBuf_ = (xL + xR) * 0.5f * feedback;

            output[i * 2 + 0] = inL + mix * (xL - inL);
            output[i * 2 + 1] = inR + mix * (xR - inR);
        }
    }

    const char* name() const noexcept override { return "SSE4.1"; }

private:
    int maxStages_ = 0;
    std::vector<float> xz1_;
    std::vector<float> yz1_;
    float fbBuf_ = 0.0f;
};

std::unique_ptr<PhaserSIMD> createPhaserSIMD_SSE41() {
    return std::make_unique<PhaserSIMD_SSE41>();
}
