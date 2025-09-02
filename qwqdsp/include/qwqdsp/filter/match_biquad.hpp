#pragma once
#include <cmath>

namespace qwqdsp {
struct MatchBiquad {
    float b0;
    float b1;
    float b2;
    float a0;
    float a1;
    float a2;

    void Lowpass(float w, float Q) noexcept {

    }

    void Highpass(float w, float Q) noexcept {

    }

    void Bandpass(float w, float Q) noexcept {

    }

    void Peak(float w, float Q, float g) noexcept {

    }

    void Lowshelf(float w, float Q, float g) noexcept {
        
    }

    void HighShelf(float w, float Q, float g) noexcept {
        
    }

    void Notch(float w, float Q) noexcept {
        
    }

    void Allpass(float w, float Q) noexcept {
        
    }
};
}