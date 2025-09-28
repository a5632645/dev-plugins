/**
 * Copyright 2024 Signalsmith Audio Ltd. / Geraint Luff
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef SIGNALSMITH_ELLIPTIC_BLEP_H
#define SIGNALSMITH_ELLIPTIC_BLEP_H

#include <array>
#include <complex>
#include <numbers>

namespace signalsmith { namespace blep {

template<class TCoeffs, class Sample, size_t kPartialLUTSize>
struct EllipticBlep {
    using Complex = std::complex<Sample>;

    void Init(Sample srate) noexcept {
        hz_to_omega_ = (2 * std::numbers::pi_v<Sample>) / srate;
        Reset();
    }

    void SetCutoff(Sample cutoff) noexcept {
        Sample scale = cutoff / 20000;

        auto addPole = [&](size_t index, Complex pole, Complex coeff, Complex impulseCoeff){
            // Set up partial powers of the pole (so we can move forward/back by fractional samples)
            for (size_t s = 0; s <= kPartialLUTSize; ++s) {
                Sample partial = Sample(s) / kPartialLUTSize;
                partial_step_poles_[s][index] = std::exp(partial * pole * hz_to_omega_);
            }

            // Impulse coeffs are always direct
            impluse_coeffs_[index] = impulseCoeff * hz_to_omega_;

            // 这是为blep服务的，升频采样不需要它们
            std::ignore = coeff;
        };

        // For now, just cast real poles to complex ones
        const auto &realCoeffs = (TCoeffs::realCoeffsDirect);
        const auto &realImpulseCoeffs = (TCoeffs::realCoeffsDirect);
        for (size_t i = 0; i < TCoeffs::realCount; ++i) {
            addPole(i, TCoeffs::realPoles[i] * scale, realCoeffs[i] * scale, realImpulseCoeffs[i] * scale);
        }
        const auto &complexCoeffs = (TCoeffs::complexCoeffsDirect);
        const auto &complexImpulseCoeffs = (TCoeffs::complexCoeffsDirect);
        for (size_t i = 0; i < TCoeffs::complexCount; ++i) {
            addPole(i + TCoeffs::realCount, TCoeffs::complexPoles[i] * scale, complexCoeffs[i] * scale, complexImpulseCoeffs[i] * scale);
        }
    }
    
    void Reset() {
        for (auto &s : state_) s = 0;
    }
    
    /// Instantaneous filter output
    Sample Get() const {
        Sample sum = 0;
        for (size_t i = 0; i < count; ++i) {
            sum += state_[i].real();
        }
        return sum;
    }

    /// Future (≤ 1 sample) filter output (as if we called `.step(samplesInFuture)` before `.get()`)
    Sample Get(Sample samplesInFuture) const {
        Sample tableIndex = samplesInFuture * kPartialLUTSize;
        size_t intIndex = static_cast<size_t>(std::floor(tableIndex));
        Sample fracIndex = tableIndex - std::floor(tableIndex);

        auto &lowPoles = partial_step_poles_[intIndex];
        auto &highPoles = partial_step_poles_[intIndex + 1];

        Sample sum = 0;
        for (size_t i = 0; i < count; ++i) {
            Complex lerpPole = lowPoles[i] + (highPoles[i] - lowPoles[i])*fracIndex;
            sum += (state_[i] * lerpPole).real();
        }
        return sum;
    }

    void Add(Sample amount) {
        for (size_t i = 0; i < count; ++i) {
            state_[i] += amount * impluse_coeffs_[i];
        }
    }
    
    void Add(Sample amount, Sample samplesInPast) {
        Sample tableIndex = samplesInPast * kPartialLUTSize;
        size_t intIndex = std::floor(tableIndex);
        Sample fracIndex = tableIndex - std::floor(tableIndex);

        // move the pulse along in time, the same way as state progresses in .step()
        auto &lowPoles = partial_step_poles_[intIndex];
        auto &highPoles = partial_step_poles_[intIndex + 1];
        for (size_t i = 0; i < count; ++i) {
            Complex lerpPole = lowPoles[i] + (highPoles[i] - lowPoles[i]) * fracIndex;
            state_[i] += impluse_coeffs_[i] * lerpPole * amount;
        }
    }

    void Step() {
        const auto &poles = partial_step_poles_.back();
        for (size_t i = 0; i < count; ++i) {
            state_[i] *= poles[i];
        }
    }

    void Step(Sample samples) {
        Sample tableIndex = samples * kPartialLUTSize;
        size_t intIndex = std::floor(tableIndex);
        Sample fracIndex = tableIndex - std::floor(tableIndex);
        // We can step forward by > 1 sample
        while (intIndex >= kPartialLUTSize) {
            Step();
            intIndex -= kPartialLUTSize;
        }

        auto &lowPoles = partial_step_poles_[intIndex];
        auto &highPoles = partial_step_poles_[intIndex + 1];

        for (size_t i = 0; i < count; ++i) {
            Complex lerpPole = lowPoles[i] + (highPoles[i] - lowPoles[i]) * fracIndex;
            state_[i] *= lerpPole;
        }
    }

private:
    // For now, just treat the real poles as complex ones
    static constexpr size_t count = TCoeffs::complexCount + TCoeffs::realCount;

    using Array = std::array<Complex, count>;
    Array state_;
    Array impluse_coeffs_;
    Sample hz_to_omega_;
    
    // Lookup table for std::pow(pole, fractional)
    std::array<Array, kPartialLUTSize + 1> partial_step_poles_;
};

}} // namespace

#endif // include guard