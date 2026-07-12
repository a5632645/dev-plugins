#pragma once
#include <memory>

namespace warpcore {
class IWarpCore {
public:
    static constexpr int kMaxBands = 256;
    static constexpr int kMaxPoles = 4;

    struct Param {
        int bands;
        float f_low;
        float f_high;
        float filter_scale;
        int filter_order;
        bool warp_first;
        float post_osc_freq_mul;
    };

    virtual ~IWarpCore() = default;

    virtual void Init(float fs) = 0;
    virtual void Reset() noexcept = 0;
    virtual void Process(float* left, float* right, int num_samples) noexcept = 0;
    virtual void Update(const Param& p) noexcept = 0;
};

std::unique_ptr<IWarpCore> CreateDsp();
}
