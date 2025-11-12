/*
 * MIT License
 * 
 * Copyright (c) 2023 Mike Jarmy
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * 
 * BSD 3-Clause License
 * 
 * Copyright (c) 2018, Roland Rabien
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 * 
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * 
 * * Neither the name of the copyright holder nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * https://github.com/FigBug/Gin/blob/master/modules/gin_dsp/dsp/gin_platereverb.h
*/

#pragma once
#include <cmath>
#include <array>
#include <memory>
#include <numbers>
#include <cassert>
#include "qwqdsp/polymath.hpp"

namespace qwqdsp::fx {
//------------------------------------------------------------------------------
// PlateReverb is an implementation of the classic plate reverb algorithm
// described by Jon Dattorro.
//
// Dattorro, J. 1997. "Effect Design Part 1: Reverberators and Other Filters."
// Journal of the Audio Engineering Society, Vol. 45, No. 9
//
// https://ccrma.stanford.edu/~dattorro/EffectDesignPart1.pdf
//
// Parameters:
//
//    mix:        Dry/wet mix.
//    predelay:   Delay before reverb.
//    lowpass:    Apply a lowpass filter before reverb.
//    decay:      How quickly the reverb decays.
//    size:       The size of our imaginary plate.
//    damping:    How much high frequencies are filtered during reverb.
//
//------------------------------------------------------------------------------

//==============================================================================
/** Plate reverb from Dattorro's paper.
 */
class PlateReverb {
public:
    static constexpr float kMaxPredelay = 0.1f; // seconds
    static constexpr float kMaxSize = 2.0f;

    // Set the sample rate.  Note that we are re-mallocing all of the various
    // delay lines here.
    void setSampleRate (float sampleRate_) {
        sampleRate = sampleRate_;

        // Ratio of our sample rate to the sample rate that is used in
        // Dattorro's paper.
        float r = float (sampleRate / 29761.0);

        // Predelay
        predelayLine.reset (new DelayLine ((size_t)std::ceil (sampleRate * kMaxPredelay)));

        // Lowpass filters
        lowpass.setSampleRate (sampleRate);
        leftTank.damping.setSampleRate (sampleRate);
        rightTank.damping.setSampleRate (sampleRate);

        // Diffusers
        diffusers[0].reset (new DelayAllpass ((size_t)std::ceil (142 * r), 0.75));
        diffusers[1].reset (new DelayAllpass ((size_t)std::ceil (107 * r), 0.75));
        diffusers[2].reset (new DelayAllpass ((size_t)std::ceil (379 * r), 0.625));
        diffusers[3].reset (new DelayAllpass ((size_t)std::ceil (277 * r), 0.625));

        // Tanks
        float maxModDepth = float (8.0 * kMaxSize * r);
        leftTank.resetDelayLines (
                                  (size_t)std::ceil (kMaxSize * 672 * r), (float)-0.7, // apf1
                                  maxModDepth,
                                  (size_t)std::ceil (kMaxSize * 4453 * r),      // del1
                                  (size_t)std::ceil (kMaxSize * 1800 * r), 0.5, // apf2
                                  (size_t)std::ceil (kMaxSize * 3720 * r)       // del2
        );
        rightTank.resetDelayLines (
                                   (size_t)std::ceil (kMaxSize * 908 * r), (float)-0.7, // apf1
                                   maxModDepth,
                                   (size_t)std::ceil (kMaxSize * 4217 * r),      // del1
                                   (size_t)std::ceil (kMaxSize * 2656 * r), 0.5, // apf2
                                   (size_t)std::ceil (kMaxSize * 3163 * r)       // del2
        );

        leftTank.lfo.setSampleRate (sampleRate);
        rightTank.lfo.setSampleRate (sampleRate);
        leftTank.lfo.setFrequency (1.0);
        rightTank.lfo.setFrequency ((float)0.95);

        // Tap points
        baseLeftTaps = {
            266 * r,  // rightTank.del1
            2974 * r, // rightTank.del1
            1913 * r, // rightTank.apf2
            1996 * r, // rightTank.del2
            1990 * r, // leftTank.del1
            187 * r,  // leftTank.apf2
            1066 * r, // leftTank.del2
        };
        baseRightTaps = {
            353 * r,  // leftTank.del1
            3627 * r, // leftTank.del1
            1228 * r, // leftTank.apf2
            2673 * r, // leftTank.del2
            2111 * r, // rightTank.del1
            335 * r,  // rightTank.apf2
            121 * r,  // rightTank.del2
        };
    }

    // Dry/wet mix.
    void setMix (float m /* [0, 1] */) {
        mix = clamp (m, 0.0, 1.0);
    }

    // Delay before reverb.
    void setPredelay (float pd /* in seconds, [0, 0.1] */) {
        predelay = clamp(pd, 0.0, kMaxPredelay) * sampleRate;
    }

    // Apply a lowpass filter before reverb.
    void setLowpass (float cutoff /* Hz */) {
        cutoff = clamp (cutoff, 16.0, 20000.0);
        lowpass.setCutoff (cutoff);
    }

    // How quickly the reverb decays.
    void setDecay (float dr /* [0, 1) */) {
        decayRate = clamp (dr, 0.0, (float)0.9999999);
        leftTank.setDecay (decayRate);
        rightTank.setDecay (decayRate);
    }

    // The size of our imaginary plate.
    //
    // The size parameter scales the delay time for all of the delay lines and
    // APFs in each tank, and for all of the tap points.
    //
    // Note that there is no size parameter in Dattorro's paper; it is an
    // extension to the original algorithm.
    void setSize (float sz /* [0, 2] */) {
        float sizeRatio = clamp(sz, 0.0, kMaxSize) / kMaxSize;

        // Scale the tank delays and APFs in each tank
        leftTank.setSizeRatio(sizeRatio);
        rightTank.setSizeRatio(sizeRatio);

        // Scale the taps
        for (size_t i = 0; i < kNumTaps; i++)
        {
            leftTaps[size_t (i)] = baseLeftTaps[size_t (i)] * sizeRatio;
            rightTaps[size_t (i)] = baseRightTaps[size_t (i)] * sizeRatio;
        }
    }

    // How much high frequencies are filtered during reverb.
    void setDamping (float cutoff /* Hz */) {
        cutoff = clamp (cutoff, 16.0, 20000.0);

        leftTank.damping.setCutoff (cutoff);
        rightTank.damping.setCutoff (cutoff);
    }

    // Process a stereo pair of samples.
    void process (float dryLeft, float dryRight, float* leftOut, float* rightOut) {
        // Note that this is "synthetic stereo".  We produce a stereo pair
        // of output samples based on the summed input.
        float sum = dryLeft + dryRight;

        // Predelay
        sum = predelayLine->tapAndPush (predelay, sum);

        // Input lowpass
        sum = lowpass.process (sum);

        // Diffusers
        sum = diffusers[0]->process (sum, (float)diffusers[0]->getSize());
        sum = diffusers[1]->process (sum, (float)diffusers[1]->getSize());
        sum = diffusers[2]->process (sum, (float)diffusers[2]->getSize());
        sum = diffusers[3]->process (sum, (float)diffusers[3]->getSize());

        // Tanks
        float leftIn = sum + rightTank.out * decayRate;
        float rightIn = sum + leftTank.out * decayRate;
        leftTank.process(leftIn);
        rightTank.process(rightIn);

        // Tap for output
        float wetLeft = rightTank.del1->tap (leftTaps[0])   //  266
                    + rightTank.del1->tap (leftTaps[1]) // 2974
                    - rightTank.apf2->tap (leftTaps[2]) // 1913
                    + rightTank.del2->tap (leftTaps[3]) // 1996
                    - leftTank.del1->tap (leftTaps[4])  // 1990
                    - leftTank.apf2->tap (leftTaps[5])  //  187
                    - leftTank.del2->tap (leftTaps[6]); // 1066

        float wetRight = leftTank.del1->tap (rightTaps[0])     //  353
                     + leftTank.del1->tap (rightTaps[1])   // 3627
                     - leftTank.apf2->tap (rightTaps[2])   // 1228
                     + leftTank.del2->tap (rightTaps[3])   // 2673
                     - rightTank.del1->tap (rightTaps[4])  // 2111
                     - rightTank.apf2->tap (rightTaps[5])  //  335
                     - rightTank.del2->tap (rightTaps[6]); //  121

        // Mix
        *leftOut = dryLeft * (1 - mix) + wetLeft * mix;
        *rightOut = dryRight * (1 - mix) + wetRight * mix;
    }

    void process (float* l, float* r, size_t num)
    {
        for (size_t i = 0; i < num; i++)
            process (l[i], r[i], &l[i], &r[i]);
    }

    void reset()
    {
        if (predelayLine)
            predelayLine->reset();

        lowpass.reset();

        for (auto& d : diffusers)
            if (d)
                d->reset();

        leftTank.reset();
        rightTank.reset();
    }

  private:

    //--------------------------------------------------------------
    // OnePoleFilter
    //--------------------------------------------------------------

    class OnePoleFilter
    {
    public:
        OnePoleFilter() {}
        ~OnePoleFilter() {}

        void setSampleRate (float sampleRate_)
        {
            sampleRate = sampleRate_;
            recalc();
        }

        void setCutoff (float cutoff_ /* Hz */)
        {
            cutoff = cutoff_;
            recalc();
        }

        float process(float x)
        {
            z = x * a + z * b;
            return z;
        }

        void reset()
        {
            a = 0;
            b = 0;
            z = 0;
        }

    private:
        float sampleRate = 1;
        float cutoff = 0;

        float a = 0;
        float b = 0;
        float z = 0;

        void recalc()
        {
            b = std::exp (-2 * std::numbers::pi_v<float> * cutoff / sampleRate);
            a = 1 - b;
        }
    };

    //--------------------------------------------------------------
    // DelayLine
    //--------------------------------------------------------------

    class DelayLine
    {
    public:
        DelayLine (size_t size_)
            : size (size_)
        {

            // For speed, create a bigger buffer than we really need.
            size_t bufferSize = ceilPowerOfTwo (size);
            buffer.reset (new float[size_t (bufferSize)]);
            std::memset (buffer.get(), 0, size_t (bufferSize) * sizeof(float));

            mask = bufferSize - 1;

            writeIdx = 0;
        }

        ~DelayLine() {}

        inline void push(float val)
        {
            buffer[size_t (writeIdx++)] = val;
            writeIdx &= mask;
        }

        inline float tap(float delay /* samples */)
        {
            // We always want to be able to properly handle any delay value that
            // gets passed in here, without going past the original size.
            assert(delay <= static_cast<float>(size));

            size_t d = (size_t)delay;
            float frac = 1 - (delay - static_cast<float>(d));

            size_t readIdx = (writeIdx - 1) - d;
            float a = buffer[size_t ((readIdx - 1) & mask)];
            float b = buffer[size_t (readIdx & mask)];

            return a + (b - a) * frac;
        }

        // This does read-before-write.
        inline float tapAndPush (float delay, float val)
        {
            float out = tap (delay);
            push (val);
            return out;
        }

        void reset()
        {
            std::memset (buffer.get(), 0, size_t (ceilPowerOfTwo (size)) * sizeof (float));
            writeIdx = 0;
        }

        inline size_t getSize() { return size; }

    private:
        const size_t size;

        std::unique_ptr<float[]> buffer;
        size_t mask;

        size_t writeIdx;

        static size_t ceilPowerOfTwo (size_t n)
        {
            return (size_t)std::pow (2, std::ceil (std::log(n) / std::log (2)));
        }
    };

    //------------------------------------------
    // DelayAllpass
    //------------------------------------------

    class DelayAllpass
    {
    public:
        DelayAllpass (size_t size_, float gain_) : delayLine(size_), gain(gain_) {}

        ~DelayAllpass() {}

        inline float process (float x, float delay)
        {
            float wd = delayLine.tap(delay);
            float w = x + gain * wd;
            float y = -gain * w + wd;
            delayLine.push(w);
            return y;
        }

        inline void setGain (float gain_) { gain = gain_; }

        inline float tap (float delay) { return delayLine.tap(delay); }

        inline size_t getSize() { return delayLine.getSize(); }

        void reset() { delayLine.reset(); }

      private:

        DelayLine delayLine;
        float gain;
    };

    //--------------------------------------------------------------
    // Lfo
    //--------------------------------------------------------------

    class Lfo
    {
    public:
        Lfo() {}
        ~Lfo() {}

        void setSampleRate (float sampleRate_)
        {
            sampleRate = sampleRate_;
            recalc();
        }

        void setFrequency (float freq_)
        {
            freq = freq_;
            recalc();
        }

        inline float process()
        {
            float out = -qwqdsp::polymath::SinParabola(phase);

            phase += phaseInc;
            if (phase > std::numbers::pi_v<float>)
                phase = -std::numbers::pi_v<float>;

            return out;
        }

        void reset()
        {
            phase = 0;
        }

    private:
        float sampleRate = 1;
        float freq = 0;

        float phaseInc = 0;
        float phase = -std::numbers::pi_v<float>;

        void recalc()
        {
            phaseInc = freq / sampleRate;
            phaseInc *= 2 * std::numbers::pi_v<float>;
        }
    };

    //------------------------------------------
    // Tank
    //------------------------------------------

    class Tank
    {
    public:

        Tank() {}
        ~Tank() {}

        void resetDelayLines (
            size_t apf1Size_, float apf1Gain_, // First APF
            float maxModDepth_,
            size_t delay1Size_,            // First delay
            size_t apf2Size_, float apf2Gain_, // Second APF
            size_t delay2Size_             // Second delay
        ) {
            apf1Size = apf1Size_;
            maxModDepth = maxModDepth_;
            float maxApf1Size = static_cast<float>(apf1Size) + maxModDepth + 1;
            apf1.reset(new DelayAllpass (size_t (maxApf1Size), apf1Gain_));

            del1.reset(new DelayLine(delay1Size_));
            apf2.reset(new DelayAllpass(apf2Size_, apf2Gain_));
            del2.reset(new DelayLine(delay2Size_));

            // We've changed the various delay line sizes and associated values,
            // so update the sizeRatio values too.
            recalcSizeRatio();
        }

        void setDecay (float decayRate_)
        {
            decayRate = decayRate_;
            apf2->setGain (clamp (float (decayRate + 0.15), 0.25, 0.5));
        }

        void setSizeRatio (float sizeRatio_)
        {
            sizeRatio = sizeRatio_;
            recalcSizeRatio();
        }

        void process (float val)
        {
            // APF1: "Controls density of tail."
            val = apf1->process(val, apf1Delay + lfo.process() * modDepth);
            val = del1->tapAndPush(del1Delay, val);

            val = damping.process(val);
            val *= decayRate;

            // APF2: "Decorrelates tank signals."
            val = apf2->process(val, apf2Delay);
            val = del2->tapAndPush(del2Delay, val);

            out = val;
        }

        void reset()
        {
            if (apf1) apf1->reset();
            if (apf2) apf2->reset();
            if (del1) del1->reset();
            if (del2) del2->reset();
            damping.reset();
            lfo.reset();
        }

        float out = 0.0;

        std::unique_ptr<DelayAllpass> apf1 = nullptr;
        std::unique_ptr<DelayAllpass> apf2 = nullptr;
        std::unique_ptr<DelayLine> del1 = nullptr;
        std::unique_ptr<DelayLine> del2 = nullptr;
        OnePoleFilter damping;
        Lfo lfo;

    private:
        size_t apf1Size = 0;
        float maxModDepth = 0;
        float modDepth = 0;

        float apf1Delay = 0;
        float apf2Delay = 0;
        float del1Delay = 0;
        float del2Delay = 0;

        float decayRate = 0;
        float sizeRatio = 0;

        void recalcSizeRatio() {

            apf1Delay = static_cast<float>(apf1Size) * sizeRatio;
            modDepth = maxModDepth * sizeRatio;

            apf2Delay = static_cast<float>(apf2->getSize()) * sizeRatio;
            del1Delay = static_cast<float>(del1->getSize()) * sizeRatio;
            del2Delay = static_cast<float>(del2->getSize()) * sizeRatio;
        }
    };

    //--------------------------------------------------------------
    //--------------------------------------------------------------
    //--------------------------------------------------------------

    float sampleRate = 1.0;

    float mix = 0.0;
    float predelay = 0.0;
    float decayRate = 0.0;

    std::unique_ptr<DelayLine> predelayLine = nullptr;
    OnePoleFilter lowpass;
    std::array<std::unique_ptr<DelayAllpass>, 4> diffusers = {
        nullptr, nullptr, nullptr, nullptr};

    Tank leftTank;
    Tank rightTank;

    static const size_t kNumTaps = 7;
    std::array<float, kNumTaps> baseLeftTaps = {};
    std::array<float, kNumTaps> baseRightTaps = {};
    std::array<float, kNumTaps> leftTaps = {};
    std::array<float, kNumTaps> rightTaps = {};

    static inline float clamp (float val, float low, float high)
    {
        return std::min (std::max (val, low), high);
    }
};
}
