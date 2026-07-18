#pragma once

#include <memory>
#include <atomic>

class PhaserSIMD;

//==============================================================================
/// High-level phaser effect processor.
///
/// Owns:
///   - dynamically-dispatched SIMD all-pass chain
///   - LFO (sine) that modulates the all-pass coefficient
///   - smoothed parameters
//==============================================================================
class PhaserProcessor {
public:
    PhaserProcessor();
    ~PhaserProcessor();

    //--------------------------------------------------------------------------
    //  Life-cycle
    //--------------------------------------------------------------------------
    void prepare(double sampleRate, int maxBlockSize);
    void reset();
    void process(float* left, float* right, int numSamples);

    //--------------------------------------------------------------------------
    //  Parameters  (set from UI thread, read in audio thread)
    //--------------------------------------------------------------------------
    void setRate(float rateHz)     noexcept { rate_.store(rateHz, std::memory_order_relaxed); }
    void setDepth(float depth)     noexcept { depth_.store(depth, std::memory_order_relaxed); }
    void setFeedback(float fb)     noexcept { feedback_.store(fb, std::memory_order_relaxed); }
    void setMix(float mix)         noexcept { mix_.store(mix, std::memory_order_relaxed); }
    void setStages(int stages)     noexcept { stages_.store(stages, std::memory_order_relaxed); }

    float getRate()     const noexcept { return rate_.load(std::memory_order_relaxed); }
    float getDepth()    const noexcept { return depth_.load(std::memory_order_relaxed); }
    float getFeedback() const noexcept { return feedback_.load(std::memory_order_relaxed); }
    float getMix()      const noexcept { return mix_.load(std::memory_order_relaxed); }
    int   getStages()   const noexcept { return stages_.load(std::memory_order_relaxed); }

    /// Name of the active SIMD backend.
    const char* backendName() const noexcept;

private:
    // ---- SIMD backend ----
    std::unique_ptr<PhaserSIMD> simd_;

    // ---- sample-rate / block ----
    double sampleRate_ = 44100.0;
    int    maxBlockSize_ = 0;

    // ---- atomic parameters (written from UI, read from audio) ----
    std::atomic<float> rate_{0.5f};
    std::atomic<float> depth_{0.5f};
    std::atomic<float> feedback_{0.3f};
    std::atomic<float> mix_{0.5f};
    std::atomic<int>   stages_{8};

    // ---- LFO state (audio thread only) ----
    double lfoPhase_ = 0.0;
};

//==============================================================================
/// Pre-defined parameter ranges.
namespace param_ranges {
inline constexpr float rateMin     = 0.05f;
inline constexpr float rateMax     = 10.0f;
inline constexpr float rateDefault = 0.5f;

inline constexpr float depthMin     = 0.0f;
inline constexpr float depthMax     = 1.0f;
inline constexpr float depthDefault = 0.5f;

inline constexpr float feedbackMin     = 0.0f;
inline constexpr float feedbackMax     = 0.95f;
inline constexpr float feedbackDefault = 0.3f;

inline constexpr float mixMin     = 0.0f;
inline constexpr float mixMax     = 1.0f;
inline constexpr float mixDefault = 0.5f;

inline constexpr int   stagesMin     = 2;
inline constexpr int   stagesMax     = 12;
inline constexpr int   stagesDefault = 8;
inline constexpr int   stagesStep    = 2;
} // namespace param_ranges
