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

    void Lowpass(float w, float Q) {

    }

    void Highpass(float w, float Q) {

    }

    void Bandpass(float w, float Q) {

    }

    void Peak(float w, float Q, float g) {

    }

    void Lowshelf(float w, float Q, float g) {
        
    }

    void HighShelf(float w, float Q, float g) {
        
    }

    void Notch(float w, float Q) {
        
    }

    void Allpass(float w, float Q) {
        
    }
};
}