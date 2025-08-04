#pragma once
#include <span>
#include <cmath>

namespace dsp {

class IGain {
public:
    virtual ~IGain() = default;
    virtual float GetPeak(int channel) = 0;
    virtual int GetNumChannels() = 0;
};

template<int NCHANNEL>
class Gain : public IGain {
public:
    static constexpr float kDecayTime = 5.0f;
    static constexpr float kMinDb = -40.0f;
    void Init(float sample_rate, int block) {
        decay_ = std::exp(-1.0f / ((sample_rate / block) * kDecayTime / 1000.0f));
    }

    void Process(std::array<std::span<float>, NCHANNEL> block) {
        for (int channel = 0; channel < NCHANNEL; ++channel) {
            float peak = peak_[channel];
            peak *= decay_;
            for (float& s : block[channel]) {
                s *= gain_;
                if (std::abs(s) > peak) peak = std::abs(s);
            }
            peak_[channel] = peak;
        }
    }

    void Process(std::span<float> block) {
        float peak = peak_[0];
        peak *= decay_;
        for (float& s : block) {
            s *= gain_;
            if (std::abs(s) > peak) peak = std::abs(s);
        }
        peak_[0] = peak;
    }

    void SetGain(float db) {
        if (db < kMinDb - 0.5f) gain_ = 0.0f;
        else gain_ = pow(10.0f, db / 20.0f);
    }

    float GetPeak(int channel) {
        return peak_[channel];
    }

    int GetNumChannels() {
        return NCHANNEL;
    }
    
    float peak_[NCHANNEL]{};
private:
    float decay_{};
    float gain_{};
};

}