#pragma once

#include <memory>

//==============================================================================
/// SIMD-accelerated phaser all-pass filter chain.
///
/// Each implementation processes two channels (stereo) through a series of
/// all-pass filters, with a shared coefficient `a` per sample.
///
/// ISA hierarchy (detected at runtime):
///   AVX2+FMA > AVX2 > AVX > SSE4.1 > SSE2 > scalar
//==============================================================================
class PhaserSIMD {
public:
    virtual ~PhaserSIMD() = default;

    /// Prepare for playback.  Must be called before processBlock().
    virtual void prepare(double sampleRate, int maxBlockSize, int maxStages) = 0;

    /// Reset internal state (delay-lines).
    virtual void reset() = 0;

    /// Process one block of audio.
    /// @param input       interleaved [L,R,L,R,…]  — can alias output
    /// @param output      interleaved [L,R,L,R,…]
    /// @param numSamples  number of stereo frames
    /// @param a           all-pass coefficient (same for all stages this frame)
    /// @param feedback    feedback amount [0…1]
    /// @param mix         wet/dry mix [0…1]
    /// @param stages      number of active all-pass stages (must be ≤ maxStages)
    virtual void processBlock(const float* input, float* output,
                              int numSamples, float a, float feedback,
                              float mix, int stages) = 0;

    /// Human-readable name of the active implementation.
    virtual const char* name() const noexcept = 0;

    //--------------------------------------------------------------------------
    //  Factory
    //--------------------------------------------------------------------------
    /// Create the best SIMD implementation for the current CPU.
    static std::unique_ptr<PhaserSIMD> create();
};

//==============================================================================
/// CPU feature-detection helpers (platform-agnostic).
namespace cpu {

/// Returns true when the CPU supports the given ISA feature set.
bool hasSSE2()    noexcept;
bool hasSSE41()   noexcept;
bool hasAVX()     noexcept;
bool hasAVX2()    noexcept;
bool hasFMA()     noexcept;
bool hasNEON()    noexcept;

} // namespace cpu
