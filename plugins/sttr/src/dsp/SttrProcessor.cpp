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
    for (int g = 0; g < kMaxGrains; ++g) {
        grainPhase_[g] = (n > 1) ? static_cast<float>(g % n) / static_cast<float>(n) : 0.0f;
    }
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
    for (int n = 0; n < numSamples; ++n) {
        // linear ramp step
        float const hopSamps = hopSmoother_.getNextValue();
        float const stretch = stretchSmoother_.getNextValue();
        float const grainLen = hopSamps * static_cast<float>(windowMul_);
        float const phaseInc = 1.0f / grainLen; // fixed window-envelope rate

        // write input (stereo)
        delayLine_.write(left[n], right[n]);

        // wet signal (all grains share the same hop/grainLen)
        float sumL = 0.0f;
        float sumR = 0.0f;
        for (int g = 0; g < N; ++g) {
            float const win = windowFn_.value(grainPhase_[g]);
            float const rphase = stretch * grainPhase_[g]; // read phase
            float const readDelay = 2.0f * rphase * grainLen; // samples behind write head

            float gl, gr;
            delayLine_.read(readDelay, gl, gr);
            sumL += win * gl;
            sumR += win * gr;

            // advance phase (same increment for all grains)
            grainPhase_[g] += phaseInc;
            if (grainPhase_[g] >= 1.0f) grainPhase_[g] -= 1.0f;
        }

        // mix dry signal from delay
        float const dryDelaySamps = grainLen * 1.5f * dryDelaySlw_.value;
        float dryL, dryR;
        delayLine_.read(dryDelaySamps, dryL, dryR);
        left[n] = interp(dryL, sumL, mixSlw_.value);
        right[n] = interp(dryR, sumR, mixSlw_.value);

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
