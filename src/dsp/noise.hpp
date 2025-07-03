#pragma once

namespace dsp {

class Noise {
public:
    void Init(float fs);
    void SetRate(float rate);
    void Reset();
    float Tick();
private:
    float fs_;
    float inc_;
    float phase_;
    float a_;
    float b_;
    float c_;
    float d_;
};

}