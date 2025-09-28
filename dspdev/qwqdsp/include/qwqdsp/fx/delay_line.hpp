#pragma once
#include <cassert>
#include <concepts>
#include <vector>
#include <cmath>
#include <array>
#include "qwqdsp/interpolation.hpp"
#include "qwqdsp/window/kaiser.hpp"

namespace qwqdsp::fx {
enum class DelayLineInterp {
    None,
    Lagrange3rd,
    PCHIP,
    Linear,
    Kaiser5,
    Kaiser9,
    Kaiser21
};

template<DelayLineInterp INTERPOLATION_TYPE = DelayLineInterp::Lagrange3rd>
class DelayLine {
public:
    void Init(float max_ms, float fs) {
        float d = max_ms * fs / 1000.0f;
        size_t i = static_cast<size_t>(std::ceil(d) + 4.0f);
        Init(i);
    }

    void Init(size_t max_samples) {
        size_t a = 1;
        while (a < max_samples) {
            a *= 2;
        }
        if (buffer_.size() < a) {
            buffer_.resize(a);
        }
        mask_ = a - 1;
        Reset();
    }

    void Reset() noexcept {
        wpos_ = 0;
        std::fill(buffer_.begin(), buffer_.end(), 0.0f);
    }

    void Push(float x) noexcept {
        buffer_[wpos_++] = x;
        wpos_ &= mask_;
    }

    float GetAfterPush(float delay_samples) noexcept {
        return Get(delay_samples + 1);
    }

    /**
     * @param delay_samples 此处不能小于1，否则为非因果滤波器（或者被绕回读取max_samples处）
     */
    float GetBeforePush(float delay_samples) noexcept {
        return Get(delay_samples);
    }

    /**
     * @param delay_samples 此处不能小于1，否则为非因果滤波器（或者被绕回读取max_samples处）
     */
    template<std::integral T>
    float GetBeforePush(T delay_samples) noexcept {
        int rpos = wpos_ + buffer_.size() - delay_samples;
        int irpos = static_cast<int>(rpos) & mask_;
        return buffer_[irpos];
    }

    template<class T, size_t N, size_t NSubSpan, double kSideLobe, double kWidthDiv>
    struct KaiserInterpolator {
        static constexpr size_t kN = N;
        static constexpr size_t kNSubSpan = NSubSpan;

        static inline const std::array<T, (NSubSpan + 2) * N> kTable = [] {
            std::array<T, N + (N - 1) * NSubSpan> coeffs{};
            double const beta = qwqdsp::window::Kaiser::Beta(kSideLobe);
            double const width = qwqdsp::window::Kaiser::MainLobeWidth(beta);
            double const cutoff = std::numbers::pi - width / kWidthDiv;

            float const center = (static_cast<float>(coeffs.size()) - 1.0f) / 2.0f;
            float const omega = cutoff / static_cast<float>(NSubSpan + 1);
            for (size_t i = 0; i < coeffs.size(); ++i) {
                float t = static_cast<float>(i) - center;
                [[unlikely]]
                if (t == 0.0f) {
                    coeffs[i] = cutoff / std::numbers::pi_v<float>;
                }
                else {
                    coeffs[i] = std::sin(omega * t) * static_cast<float>(NSubSpan + 1) / (std::numbers::pi_v<float> * t);
                }
            }

            qwqdsp::window::Kaiser::ApplyWindow(coeffs, beta, false);

            std::array<T, (NSubSpan + 2) * N> table{};
            for (size_t i = 0; i < NSubSpan + 1; ++i) {
                size_t const aa = i == 0 ? N : N - 1;
                for (size_t j = 0; j < aa; ++j) {
                    table[i * N + j] = coeffs[i + (NSubSpan + 1) * j];
                }
            }
            for (size_t i = 0; i < N - 1; ++i) {
                table[(NSubSpan + 1) * N + i] = table[i + 1];
            }
            return table;
        }();
    };
private:
    float Get(float delay) noexcept {
        if constexpr (INTERPOLATION_TYPE == DelayLineInterp::None) {
            float rpos = wpos_ + buffer_.size() - delay;
            int irpos = static_cast<int>(std::round(rpos)) & mask_;
            return buffer_[irpos];
        }
        else {
            float rpos = wpos_ + buffer_.size() - delay;
            int irpos = static_cast<int>(rpos) & mask_;
            [[maybe_unused]] int inext1 = (irpos + 1) & mask_;
            [[maybe_unused]] int inext2 = (irpos + 2) & mask_;
            [[maybe_unused]] int inext3 = (irpos + 3) & mask_;
            [[maybe_unused]] int iprev1 = (irpos - 1) & mask_;
            [[maybe_unused]] float t = rpos - static_cast<int>(rpos);
            if constexpr (INTERPOLATION_TYPE == DelayLineInterp::Lagrange3rd) {
                return Interpolation::Lagrange3rd(buffer_[irpos], buffer_[inext1], buffer_[inext2], buffer_[inext3], t);
            }
            else if constexpr (INTERPOLATION_TYPE == DelayLineInterp::Linear) {
                return Interpolation::Linear(buffer_[irpos], buffer_[inext1], t);
            }
            else if constexpr (INTERPOLATION_TYPE == DelayLineInterp::PCHIP) {
                return Interpolation::PCHIP(buffer_[iprev1], buffer_[irpos], buffer_[inext1], buffer_[inext2], t);
            }
            else if constexpr (INTERPOLATION_TYPE == DelayLineInterp::Kaiser5) {
                static KaiserInterpolator<float, 5, 127, 70.0, 1.8> table;
                float const phase = rpos;
                size_t const center = static_cast<size_t>(phase);
                float const frac = phase - std::floor(phase);
                float const span_idx = frac * (table.kNSubSpan + 1);
                size_t const lower = static_cast<size_t>(span_idx);
                size_t higher = lower + 1;
                float const span_frac = span_idx - lower;

                size_t const xbeing = center + (table.kN - 1) / 2;
                float sum{};
                for (size_t i = 0; i < table.kN; ++i) {
                    size_t const xpos = (xbeing - i) & mask_;
                    float const coeff0 = table.kTable[lower * table.kN + i];
                    float const coeff1 = table.kTable[higher * table.kN + i];
                    float const coeff = Interpolation::Linear(coeff0, coeff1, span_frac);
                    sum += coeff * buffer_[xpos];
                }
                return sum;
            }
            else if constexpr (INTERPOLATION_TYPE == DelayLineInterp::Kaiser9) {
                static KaiserInterpolator<float, 9, 127, 60.0, 1.8> table;
                float const phase = rpos;
                size_t const center = static_cast<size_t>(phase);
                float const frac = phase - std::floor(phase);
                float const span_idx = frac * (table.kNSubSpan + 1);
                size_t const lower = static_cast<size_t>(span_idx);
                size_t higher = lower + 1;
                float const span_frac = span_idx - lower;

                size_t const xbeing = center + (table.kN - 1) / 2;
                float sum{};
                for (size_t i = 0; i < table.kN; ++i) {
                    size_t const xpos = (xbeing - i) & mask_;
                    float const coeff0 = table.kTable[lower * table.kN + i];
                    float const coeff1 = table.kTable[higher * table.kN + i];
                    float const coeff = Interpolation::Linear(coeff0, coeff1, span_frac);
                    sum += coeff * buffer_[xpos];
                }
                return sum;
            }
            else if constexpr (INTERPOLATION_TYPE == DelayLineInterp::Kaiser21) {
                static KaiserInterpolator<float, 21, 127, 60.0, 2.0> table;
                float const phase = rpos;
                size_t const center = static_cast<size_t>(phase);
                float const frac = phase - std::floor(phase);
                float const span_idx = frac * (table.kNSubSpan + 1);
                size_t const lower = static_cast<size_t>(span_idx);
                size_t higher = lower + 1;
                float const span_frac = span_idx - lower;

                size_t const xbeing = center + (table.kN - 1) / 2;
                float sum{};
                for (size_t i = 0; i < table.kN; ++i) {
                    size_t const xpos = (xbeing - i) & mask_;
                    float const coeff0 = table.kTable[lower * table.kN + i];
                    float const coeff1 = table.kTable[higher * table.kN + i];
                    float const coeff = Interpolation::Linear(coeff0, coeff1, span_frac);
                    sum += coeff * buffer_[xpos];
                }
                return sum;
            }
        }
    }

    std::vector<float> buffer_;
    size_t wpos_{};
    size_t mask_{};
};
}