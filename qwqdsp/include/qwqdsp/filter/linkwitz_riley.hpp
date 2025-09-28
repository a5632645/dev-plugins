#pragma once
#include <utility>
#include "qwqdsp/filter/biquad.hpp"
#include "qwqdsp/filter/rbj.hpp"
#include "qwqdsp/convert.hpp"

namespace qwqdsp::filter {
class LinkwitzRiley2 {
public:
    void Reset() noexcept {
        lp_.Reset();
        hp_.Reset();
    }

    std::pair<float, float> Tick(float x) noexcept {
        return {lp_.Tick(x), -hp_.Tick(x)};
    }

    void SetCutoff(float f, float fs) noexcept {
        RBJ design;
        design.Lowpass(qwqdsp::convert::Freq2W(f, fs), 0.5f);
        lp_.Set(design.b0, design.b1, design.b2, design.a1, design.a2);
        design.Highpass(qwqdsp::convert::Freq2W(f, fs), 0.5f);
        hp_.Set(design.b0, design.b1, design.b2, design.a1, design.a2);
    }
private:
    Biquad lp_;
    Biquad hp_;
};

class LinkwitzRiley4 {
public:
    void Reset() noexcept {
        lp_.Reset();
        lp2_.Reset();
        hp_.Reset();
        hp2_.Reset();
    }

    /**
     * @return [lp, hp]
     */
    std::pair<float, float> Tick(float x) noexcept {
        return {lp_.Tick(lp2_.Tick(x)), hp_.Tick(hp2_.Tick(x))};
    }

    void SetCutoff(float f, float fs) noexcept {
        RBJ design;
        design.Lowpass(qwqdsp::convert::Freq2W(f, fs), std::numbers::sqrt2_v<float> * 0.5f);
        lp_.Set(design.b0, design.b1, design.b2, design.a1, design.a2);
        lp2_.Set(design.b0, design.b1, design.b2, design.a1, design.a2);
        design.Highpass(qwqdsp::convert::Freq2W(f, fs), std::numbers::sqrt2_v<float> * 0.5f);
        hp_.Set(design.b0, design.b1, design.b2, design.a1, design.a2);
        hp2_.Set(design.b0, design.b1, design.b2, design.a1, design.a2);
    }
private:
    Biquad lp_;
    Biquad lp2_;
    Biquad hp_;
    Biquad hp2_;
};

class LinkwitzRiley6 {
public:
    void Reset() noexcept {
        lp_.Reset();
        lp2_.Reset();
        lp3_.Reset();
        hp_.Reset();
        hp2_.Reset();
        hp3_.Reset();
    }

    /**
     * @return [lp, hp]
     */
    std::pair<float, float> Tick(float x) noexcept {
        return {
            lp_.Tick(lp2_.Tick(lp3_.Tick(x))),
            -hp_.Tick(hp2_.Tick(hp3_.Tick(x)))
        };
    }

    void SetCutoff(float f, float fs) noexcept {
        RBJ design;
        design.Lowpass(qwqdsp::convert::Freq2W(f, fs), 1.0f);
        lp_.Set(design.b0, design.b1, design.b2, design.a1, design.a2);
        lp2_.Set(design.b0, design.b1, design.b2, design.a1, design.a2);
        design.Lowpass(qwqdsp::convert::Freq2W(f, fs), 0.5f);
        lp3_.Set(design.b0, design.b1, design.b2, design.a1, design.a2);
        design.Highpass(qwqdsp::convert::Freq2W(f, fs), 1.0f);
        hp_.Set(design.b0, design.b1, design.b2, design.a1, design.a2);
        hp2_.Set(design.b0, design.b1, design.b2, design.a1, design.a2);
        design.Highpass(qwqdsp::convert::Freq2W(f, fs), 0.5f);
        hp3_.Set(design.b0, design.b1, design.b2, design.a1, design.a2);
    }
private:
    Biquad lp_;
    Biquad lp2_;
    Biquad lp3_;
    Biquad hp_;
    Biquad hp2_;
    Biquad hp3_;
};

class LinkwitzRiley8 {
public:
    void Reset() noexcept {
        lp_.Reset();
        lp2_.Reset();
        lp3_.Reset();
        lp4_.Reset();
        hp_.Reset();
        hp2_.Reset();
        hp3_.Reset();
        hp4_.Reset();
    }

    /**
     * @return [lp, hp]
     */
    std::pair<float, float> Tick(float x) noexcept {
        return {
            lp_.Tick(lp2_.Tick(lp3_.Tick(lp4_.Tick(x)))),
            hp_.Tick(hp2_.Tick(hp3_.Tick(hp4_.Tick(x))))
        };
    }

    void SetCutoff(float f, float fs) noexcept {
        RBJ design;
        design.Lowpass(qwqdsp::convert::Freq2W(f, fs), 0.54119610f);
        lp_.Set(design.b0, design.b1, design.b2, design.a1, design.a2);
        lp2_.Set(design.b0, design.b1, design.b2, design.a1, design.a2);
        design.Lowpass(qwqdsp::convert::Freq2W(f, fs), 1.3065630f);
        lp3_.Set(design.b0, design.b1, design.b2, design.a1, design.a2);
        lp4_.Set(design.b0, design.b1, design.b2, design.a1, design.a2);
        design.Highpass(qwqdsp::convert::Freq2W(f, fs), 0.54119610f);
        hp_.Set(design.b0, design.b1, design.b2, design.a1, design.a2);
        hp2_.Set(design.b0, design.b1, design.b2, design.a1, design.a2);
        design.Highpass(qwqdsp::convert::Freq2W(f, fs), 1.3065630f);
        hp3_.Set(design.b0, design.b1, design.b2, design.a1, design.a2);
        hp4_.Set(design.b0, design.b1, design.b2, design.a1, design.a2);
    }
private:
    Biquad lp_;
    Biquad lp2_;
    Biquad lp3_;
    Biquad lp4_;
    Biquad hp_;
    Biquad hp2_;
    Biquad hp3_;
    Biquad hp4_;
};
}