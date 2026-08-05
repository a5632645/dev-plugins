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

    // Allocate stereo delay line (4× max hop × max grains)
    delayCap_ = static_cast<int>(4.0f * kMaxHopMs * kMaxGrains * sampleRate_ / 1000.0f);
    for (auto& buf : delayBuf_) buf.assign(static_cast<size_t>(delayCap_), 0.0f);

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
    for (auto& buf : delayBuf_) std::fill(buf.begin(), buf.end(), 0.0f);

    delayWriter_ = 0;
    masterWPos_ = 0.0f;
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
    stretchSmoother_.setTargetValue(std::clamp(stretch_, 0.7f, 1.4f));
}

void SttrProcessor::syncGrains() {
    if (std::abs(windowFn_.beta() - windowBeta_) > 1.0e-6f)
        windowFn_.setBeta(windowBeta_);

    int const n = grainsForMul(windowMul_);
    if (n != numGrains_) {
        numGrains_ = n;
        reset();
    }
}

//==============================================================================
template <int N>
void SttrProcessor::processGrains(float* left, float* right, int numSamples) {
    unsigned int const delayLen = static_cast<unsigned int>(delayCap_);
    float* bufL = delayBuf_[0].data();
    float* bufR = delayBuf_[1].data();

    for (int n = 0; n < numSamples; ++n) {
        // linear ramp step
        float const hopSamps = hopSmoother_.getNextValue();
        float const stretch = stretchSmoother_.getNextValue();
        float const grainLen = hopSamps * static_cast<float>(windowMul_);
        float const phaseInc = stretch / grainLen;

        // write input
        bufL[delayWriter_] = left[n];
        bufR[delayWriter_] = right[n];

        // wet signal (all grains share the same hop/grainLen)
        float sumL = 0.0f;
        float sumR = 0.0f;
        for (int g = 0; g < N; ++g) {
            float readPos = wrap(masterWPos_ - 2.0f * grainPhase_[g] * grainLen, static_cast<float>(delayLen));
            float win = windowFn_.value(grainPhase_[g]);

            sumL += win * interpSample(bufL, readPos, delayLen);
            sumR += win * interpSample(bufR, readPos, delayLen);

            // advance phase (same increment for all grains)
            grainPhase_[g] += phaseInc;
            if (grainPhase_[g] >= 1.0f) grainPhase_[g] -= 1.0f;
        }

        // mix dry signal from delay
        float dryReader = wrap(masterWPos_ - grainLen * 1.5f * dryDelaySlw_.value, static_cast<float>(delayLen));
        float dryL = interpSample(bufL, dryReader, delayLen);
        float dryR = interpSample(bufR, dryReader, delayLen);
        left[n] = interp(dryL, sumL, mixSlw_.value);
        right[n] = interp(dryR, sumR, mixSlw_.value);

        // advance state
        ++delayWriter_;
        if (delayWriter_ >= delayCap_) delayWriter_ = 0;
        masterWPos_ += 1.0f;
        if (masterWPos_ >= static_cast<float>(delayCap_)) masterWPos_ -= static_cast<float>(delayCap_);

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
