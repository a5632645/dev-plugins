#include "phaser_simd.hpp"

// Forward declaration
std::unique_ptr<PhaserSIMD> createPhaserSIMD_NEON();

// NEON is ARM-only — skip on other architectures to avoid arm_neon.h errors
#if defined(__ARM_NEON) || defined(__ARM_NEON__) || defined(__aarch64__) || defined(_M_ARM64)

#include <cstring>
#include <algorithm>
#include <vector>
#include <arm_neon.h>

//==============================================================================
//  ARM NEON phaser implementation
//  Processes both stereo channels in parallel using 128-bit NEON vectors.
//  Layout: float32x4_t = [L, R, —, —] (lower 2 lanes used)
//==============================================================================

class PhaserSIMD_NEON final : public PhaserSIMD {
public:
    void prepare(double /*sampleRate*/, int /*maxBlockSize*/, int maxStages) override {
        maxStages_ = maxStages;
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

        const float32x4_t va    = vdupq_n_f32(a);
        const float32x4_t vfb   = vdupq_n_f32(feedback);
        const float32x4_t vMix  = vdupq_n_f32(mix);
        const float32x4_t vHalf = vdupq_n_f32(0.5f);

        for (int i = 0; i < numSamples; ++i) {
            float inL = input[i * 2 + 0];
            float inR = input[i * 2 + 1];

            float xL = inL + feedback * fbBuf_;
            float xR = inR + feedback * fbBuf_;

            for (int st = 0; st < s; ++st) {
                const int idx = st * 2;

                float32x4_t vxz1 = vld1q_f32(&xz1_[idx]);
                float32x4_t vyz1 = vld1q_f32(&yz1_[idx]);
                float32x4_t vx   = {xL, xR, 0.0f, 0.0f};

                // y = xz1 + a * (yz1 - x)   via FMA
                float32x4_t vdiff = vsubq_f32(vyz1, vx);
                float32x4_t vy = vmlaq_f32(vxz1, va, vdiff);

                vst1q_f32(&xz1_[idx], vx);
                vst1q_f32(&yz1_[idx], vy);

                float tmp[4];
                vst1q_f32(tmp, vy);
                xL = tmp[0];
                xR = tmp[1];
            }

            fbBuf_ = (xL + xR) * 0.5f * feedback;

            output[i * 2 + 0] = inL + mix * (xL - inL);
            output[i * 2 + 1] = inR + mix * (xR - inR);
        }
    }

    const char* name() const noexcept override { return "NEON"; }

private:
    int maxStages_ = 0;
    std::vector<float> xz1_;
    std::vector<float> yz1_;
    float fbBuf_ = 0.0f;
};

std::unique_ptr<PhaserSIMD> createPhaserSIMD_NEON() {
    return std::make_unique<PhaserSIMD_NEON>();
}

#else  // not ARM
// Stub — this translation unit still provides the symbol so the factory
// extern declaration (guarded by the same macro) can link when it's active.
std::unique_ptr<PhaserSIMD> createPhaserSIMD_NEON() {
    return nullptr;
}
#endif
