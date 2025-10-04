#pragma once
#include <cmath>
#include <xsimd/xsimd.hpp>

class AllpassFilter {
public:
    AllpassFilter() = default;
    AllpassFilter(float theta, float pole_radius) {
        SetCoefficients(theta, pole_radius);
    }

    void SetAdditionalInfo(float center, float bw) {
        center_ = center;
        bw_ = bw;
    }

    float GetCenter() const {
        return center_;
    }

    float GetBw() const {
        return bw_;
    }

    void SetCoefficients(float theta, float pole_radius) {
        jassert(pole_radius >= 0 && pole_radius <= 1);

        pole_radius_ = pole_radius;
        coeff_[0] = pole_radius * pole_radius;
        coeff_[1] = -2 * pole_radius * std::cos(theta);
        coeff_[2] = -coeff_[1];
        coeff_[3] = -coeff_[0];
    }

    float Process(float input) {
        auto x1 = xy_[1];
        xy_[1] = xy_[0];
        xy_[0] = input;
        
        using batch = xsimd::batch<float, xsimd::sse2>;
        auto xv = batch::load_aligned(&xy_[0]);
        auto cv = batch::load_aligned(&coeff_[0]);
        auto output = x1 + xsimd::reduce_add(xv * cv);

        xy_[3] = xy_[2];
        xy_[2] = output;
        return output;
    }

    float GetGroupDelay(float w) const {
        constexpr auto interval = 1.0f / 100000.0f;
        return -(GetPhaseResponse(w + interval) - GetPhaseResponse(w)) / interval;
    }

    float GetPhaseResponse(float w) const {
        return -2 * w
            -2 * std::atan(pole_radius_ * std::sin(w - center_) / (1 - pole_radius_ * std::cos(w - center_)))
            - std::atan(pole_radius_ * std::sin(w + center_) / (1 - pole_radius_ * std::cos(w + center_)));
    }
private:
    __declspec(align(16)) float xy_[4]{};
    __declspec(align(16)) float coeff_[4]{};

    float center_{};
    float bw_{};
    float pole_radius_{};
};