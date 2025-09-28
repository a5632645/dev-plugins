#pragma once
#include <vector>
#include <span>
#include "../source/elliptic_blep.hpp"

namespace qwqdsp::fx {
/**
 * @brief holters-parker IIR重采样器，使用Elliptic-blep库实现，移除了高通滤波器系数
 */
template<class TCoeff, size_t kPartialStep>
class ResampleIIR {
public:
    using T = typename TCoeff::TSample;

    void Init(T source_fs, T target_fs) {
        blep_.Init(source_fs);
        blep_.SetCutoff(target_fs / 2 * TCoeff::fpass / TCoeff::fstop);
        phase_inc_ = source_fs / target_fs;
    }

    template<std::floating_point IOSample>
    std::vector<IOSample> Process(std::span<IOSample> x) {
        std::vector<IOSample> ret;

        blep_.Reset();

        T phase{};
        size_t rpos{};
        blep_.Add(static_cast<T>(x[0]));
        while (rpos < x.size() - 1) {
            T const frac = phase;
            T const v = blep_.Get(frac);
            ret.push_back(static_cast<IOSample>(v));

            phase += phase_inc_;
            size_t new_rpos = rpos + static_cast<size_t>(std::floor(phase));
            phase -= std::floor(phase);

            new_rpos = std::min(new_rpos, x.size() - 1);
            for (size_t i = rpos; i < new_rpos; ++i) {
                blep_.Step();
                blep_.Add(static_cast<T>(x[i + 1]));
            }
            rpos = new_rpos;
        }

        return ret;
    }
private:
    T phase_inc_{};
    signalsmith::blep::EllipticBlep<TCoeff, T, kPartialStep> blep_;
};
}