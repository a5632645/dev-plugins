#include "phaser_simd.hpp"

#include <cstring>
#include <algorithm>
#include <vector>
// no direct use of <cmath> in this file

//==============================================================================
//  Scalar (portable) implementation — no SIMD
//==============================================================================

class PhaserSIMD_Scalar final : public PhaserSIMD {
public:
    void prepare(double /*sampleRate*/, int /*maxBlockSize*/, int maxStages) override {
        maxStages_ = maxStages;
        // 2 channels per stage
        const size_t sz = static_cast<size_t>(maxStages) * 2;
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

        for (int i = 0; i < numSamples; ++i) {
            const float inL = input[i * 2 + 0];
            const float inR = input[i * 2 + 1];

            // ---- all-pass chain ----
            float xL = inL + feedback * fbBuf_;
            float xR = inR + feedback * fbBuf_;
            float yL, yR;

            for (int st = 0; st < s; ++st) {
                const auto idx = static_cast<size_t>(st) * 2;

                // Left
                yL = xz1_[idx] + a * (yz1_[idx] - xL);
                xz1_[idx] = xL;
                yz1_[idx] = yL;
                xL = yL;

                // Right
                yR = xz1_[idx + 1] + a * (yz1_[idx + 1] - xR);
                xz1_[idx + 1] = xR;
                yz1_[idx + 1] = yR;
                xR = yR;
            }

            fbBuf_ = (xL + xR) * 0.5f * feedback;

            // Wet/dry mix
            output[i * 2 + 0] = inL + mix * (xL - inL);
            output[i * 2 + 1] = inR + mix * (xR - inR);
        }
    }

    const char* name() const noexcept override { return "Scalar"; }

private:
    int maxStages_ = 0;
    std::vector<float> xz1_;  // x[n-1] per stage × channel
    std::vector<float> yz1_;  // y[n-1] per stage × channel
    float fbBuf_ = 0.0f;      // feedback buffer
};

//==============================================================================
//  Factory
//==============================================================================

std::unique_ptr<PhaserSIMD> PhaserSIMD::create() {
    // Try best available, in descending order.
    // Each implementation is declared in its own translation unit.
    //
    // We use extern declarations so the linker picks them up
    // only when the corresponding .cpp is compiled.

#if defined(__ARM_NEON) || defined(__ARM_NEON__) || defined(__aarch64__)
    if (cpu::hasNEON()) {
        extern std::unique_ptr<PhaserSIMD> createPhaserSIMD_NEON();
        return createPhaserSIMD_NEON();
    }
#endif

#if defined(__x86_64__) || defined(_M_X64)
    if (cpu::hasAVX2() && cpu::hasFMA()) {
        extern std::unique_ptr<PhaserSIMD> createPhaserSIMD_AVX2_FMA();
        return createPhaserSIMD_AVX2_FMA();
    }
    if (cpu::hasAVX2()) {
        extern std::unique_ptr<PhaserSIMD> createPhaserSIMD_AVX2();
        return createPhaserSIMD_AVX2();
    }
    if (cpu::hasAVX()) {
        extern std::unique_ptr<PhaserSIMD> createPhaserSIMD_AVX();
        return createPhaserSIMD_AVX();
    }
    if (cpu::hasSSE41()) {
        extern std::unique_ptr<PhaserSIMD> createPhaserSIMD_SSE41();
        return createPhaserSIMD_SSE41();
    }
    if (cpu::hasSSE2()) {
        extern std::unique_ptr<PhaserSIMD> createPhaserSIMD_SSE2();
        return createPhaserSIMD_SSE2();
    }
#endif

    // Fallback to scalar
    return std::make_unique<PhaserSIMD_Scalar>();
}
