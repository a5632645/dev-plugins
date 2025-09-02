#pragma once
#include <cstddef>
#include <array>
#include <cmath>

namespace qwqdsp {
template<size_t N>
class ThiranFilter {
public:
    void Reset() noexcept {
        std::fill(x_.begin(), x_.end(), 0.0f);
        std::fill(y_.begin(), y_.end(), 0.0f);
    }

    /**
    * @return how many intergal samples need delay
    */
    size_t Make(float delay) noexcept {
        delay += 0.5f;
        float frac = delay - std::floor(delay);
        float frac_thiran = frac - 0.5f;
        int ret = std::round(delay - 0.5f - frac_thiran - N);
        if (ret < 0) ret = 0;

        for (size_t k = 1; k <= N; ++k) {
            float sign = k % 2 == 0 ? 1.0f : -1.0f;
            float nchoose = kNChooseTable[k - 1];
            float mul = NMul(k, frac_thiran);
            a_[k - 1] = sign * nchoose * mul;
        }

        return ret;
    }

    float Tick(float x) noexcept {
        float y = x_.back();
        for (size_t i = N - 1; i != 0; --i) {
            x_[i] = x_[i - 1];
        }
        x_[0] = x;

        size_t ii = N - 1;
        for (size_t i = 0; i < N; ++i) {
            y += a_[ii] * x_[i];
            y -= a_[i] * y_[i];
            --ii;
        }

        for (size_t i = N - 1; i != 0; --i) {
            y_[i] = y_[i - 1];
        }
        y_[0] = y;
        return y;
    }
private:
    static float NMul(size_t k, float frac) noexcept {
        float s = 1.0f;
        for (size_t n = 0; n <= N; ++n) {
            s *= (frac + n) / (frac + k + n);
        }
        return s;
    }

    static constexpr size_t NChoose(size_t n, size_t k) noexcept {
        if (k > n) {
            return 0;
        }
        size_t result = 1;
        for (size_t i = 1; i <= k; ++i) {
            result = result * (n - i + 1) / i;
        }
        return result;
    }

    static constexpr auto kNChooseTable = []{
        std::array<size_t, N> r;
        for (size_t i = 0; i < N; ++i) {
            r[i] = NChoose(N, i + 1);
        }
        return r;
    }();

    std::array<float, N> x_{};
    std::array<float, N> y_{};
    std::array<float, N> a_{};
};
}
