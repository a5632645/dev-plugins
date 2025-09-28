#pragma once
#include <cmath>
#include <Eigen/Dense>

namespace qwqdsp::adaptive {
// qwqfixme: 不工作
template <int ORDER>
class RLSFIlter {
public:
    using vec = Eigen::Matrix<double, ORDER, 1>;
    using mat = Eigen::Matrix<double, ORDER, ORDER>;

    void Init(double identity = 0.01) noexcept {
        identity_val_ = identity;
        Reset();
    }

    void Reset() noexcept {
        identity_.setIdentity();
        p_ = identity_ * identity_val_;
        w_.setZero();
        latch_.setZero();
        iir_latch_.setZero();
    }

    /**
     * @return 预测值
     */
    double Tick(double source, double target) noexcept {
        for (int i = ORDER - 1; i > 0; --i) {
            latch_[i] = latch_[i - 1];
        }
        latch_[0] = source;

        // prediate
        double const pred = w_.transpose() * latch_;
        err_ = target - pred;

        double const lamda_inv = 1.0 / forget_;
        p2_.noalias() = p_ * lamda_inv;
        k_.noalias() = (p_ * latch_) / (forget_ + (latch_.transpose() * p_ * latch_).value());
        p_.noalias() = (identity_ - k_ * latch_.transpose()) * p2_;
        w_.noalias() += k_ * err_;
        
        return pred;
    }

    /**
     * @brief 使用滤波器参数计算一次，与预测lag分离的存储
     */
    double Filter(double x) noexcept {
        // iir processing
        double gain = std::sqrt(err_ * err_ + 1e-18f);

        double yy = w_.transpose() * iir_latch_ + gain * x;
        for (int i = ORDER - 1; i > 0; --i) {
            iir_latch_[i] = iir_latch_[i - 1];
        }
        iir_latch_[0] = yy;
        return yy;
    }

    void SetForgetParam(float forget) noexcept {
        forget_ = forget;
    }
private:
    double forget_ = 0.999;
    double err_{};
    double identity_val_{};
    mat p_;
    mat p2_;
    mat identity_;
    vec w_;
    vec latch_;
    vec k_;

    vec iir_latch_;
};
}