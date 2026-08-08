#include "SttrProcessor.hpp"

//==============================================================================
void SttrProcessor::setParameters(Parameters const& params) {
    mix_ = params.mix;
    hopMs_ = params.hopMs;
    dryDelay_ = params.dryDelay;
    stretch_ = params.stretch;

    windowMul_ = params.windowMul;
    windowBeta_ = params.windowBeta;

    // setParameters() is only called when a parameter actually changes, so
    // pull the smoother targets and re-sync the grain count here.
    pullTargets();
    syncGrains();
}

void SttrProcessor::prepare(float sampleRate) {
    sampleRate_ = sampleRate;

    // Size the delay line for the worst-case read depth
    delayLine_.Init(kMaxDelayMs, sampleRate);

    // Initial hop value and 40ms linear ramp
    float initHop = millisecondsToSamples(hopMs_, sampleRate_);
    hopSmoother_.reset(sampleRate_, 0.04);
    hopSmoother_.setCurrentAndTargetValue(initHop);

    stretchSmoother_.reset(sampleRate_, 0.04);
    stretchSmoother_.setCurrentAndTargetValue(stretch_);

    reset();
}

//==============================================================================
void SttrProcessor::reset() {
    delayLine_.clear();

    lfoPhase_ = 0.0f;

    // Initialise per-grain phases (staggered evenly)
    int const n = numGrains_ > 0 ? numGrains_ : 2;
    simd::Float128 init{};
    for (int g = 0; g < kMaxGrains; ++g) init[g] = (n > 1) ? static_cast<float>(g % n) / static_cast<float>(n) : 0.0f;
    grainPhase_ = init;
}

//==============================================================================
void SttrProcessor::pullTargets() {
    mixSlw_.target = mix_;
    hopSmoother_.setTargetValue(std::max(millisecondsToSamples(hopMs_, sampleRate_), 1.0f));
    dryDelaySlw_.target = dryDelay_;
    // stretch ratio = 2^(formant/12), formant in [-10, 10] st
    stretchSmoother_.setTargetValue(std::clamp(stretch_, 0.561231f, 1.781797f));
}

void SttrProcessor::syncGrains() {
    if (std::abs(windowFn_.beta() - windowBeta_) > 1.0e-6f) windowFn_.setBeta(windowBeta_);

    int const n = grainsForMul(windowMul_);
    if (n != numGrains_) {
        numGrains_ = n;
        reset();
    }
}

//==============================================================================
template <int N>
void SttrProcessor::processGrains(float* left, float* right, int numSamples) {
    // Grains g >= N are inactive: masked out of the wet sum and their phases stay
    // frozen (matching the scalar per-grain advance loop).
    simd::Float128 const laneMask =
        simd::Float128{(N > 0) ? 1.0f : 0.0f, (N > 1) ? 1.0f : 0.0f, (N > 2) ? 1.0f : 0.0f, (N > 3) ? 1.0f : 0.0f};

    for (int n = 0; n < numSamples; ++n) {
        // linear ramp step
        float const hopSamps = hopSmoother_.getNextValue();
        float const stretch = stretchSmoother_.getNextValue();
        float const grainLen = hopSamps * static_cast<float>(windowMul_);
        float const phaseInc = 1.0f / grainLen; // fixed window-envelope rate

        // write input (stereo)
        delayLine_.write(left[n], right[n]);

        // wet signal: all grains advance in lockstep with the same phaseInc, so pack
        // the per-grain phases into one 4-lane vector and process them together.
        simd::Float128 const phase = grainPhase_;

        // Kaiser window — SIMD overload, bit-identical per lane to the scalar version
        simd::Float128 const win = windowFn_.value(phase);

        // read delay = 2 * (stretch * phase) * grainLen, samples behind the write head
        simd::Float128 const readDelay = simd::BroadcastF128(2.0f) * (stretch * phase) * simd::BroadcastF128(grainLen);

        // wet reads: the templated read<N> only reads the first N lanes and leaves
        // lanes >= N at zero, so the vector reduce below stays valid for any N.
        simd::Float128 gl{};
        simd::Float128 gr{};
        delayLine_.read<N>(readDelay, gl, gr);
        float const sumL = simd::ReduceAdd(win * gl * laneMask);
        float const sumR = simd::ReduceAdd(win * gr * laneMask);

        // advance phases (inactive lanes stay frozen)
        grainPhase_ = simd::Frac(phase + simd::BroadcastF128(phaseInc));

        // mix dry signal from delay (single delay -> read<1>)
        float const dryDelaySamps = grainLen * 1.5f * dryDelaySlw_.value;
        simd::Float128 dryL, dryR;
        delayLine_.read<1>(simd::Float128{dryDelaySamps, 0.0f, 0.0f, 0.0f}, dryL, dryR);
        left[n] = interp(dryL[0], sumL, mixSlw_.value);
        right[n] = interp(dryR[0], sumR, mixSlw_.value);

        dryDelaySlw_.step();
    }

    mixSlw_.step();
}

//------------------------------------------------------------------------------
template void SttrProcessor::processGrains<1>(float*, float*, int);
template void SttrProcessor::processGrains<2>(float*, float*, int);
template void SttrProcessor::processGrains<3>(float*, float*, int);
template void SttrProcessor::processGrains<4>(float*, float*, int);

//==============================================================================
void SttrProcessor::processBlock(float* left, float* right, int numSamples) {
    switch (numGrains_) {
        case 1:
            processGrains<1>(left, right, numSamples);
            break;
        case 2:
            processGrains<2>(left, right, numSamples);
            break;
        case 3:
            processGrains<3>(left, right, numSamples);
            break;
        case 4:
            processGrains<4>(left, right, numSamples);
            break;
        default:
            processGrains<4>(left, right, numSamples);
            break;
    }
}
