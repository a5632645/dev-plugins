#pragma once
#include <cstddef>
#include <numbers>
#include <span>
#include <cmath>

namespace qwqdsp::window {
/**
 * @ref http://practicalcryptography.com/miscellaneous/machine-learning/implementing-dolph-chebyshev-window/
 */
struct Chebyshev {
    /**
     * @param atten >0
     * @note 如果要用于分析，请使用N+1的窗然后丢弃最后一个样本
     */
    static void Window(std::span<float> x, float atten) {
        const size_t N = x.size();
        float tg = std::pow(10,atten/20);
        float x0 = std::cosh((1.0 / (N - 1)) * std::acosh(tg));
        float M = (N - 1.0f) / 2.0f;
        if(N % 2 == 0) {
            M = M + 0.5;
        }
        constexpr float pi = std::numbers::pi_v<float>;
        for(size_t nn = 0; nn < (N / 2 + 1); ++nn){
            float n = nn - M;
            float sum = 0;
            for(size_t i = 1; i <= static_cast<size_t>(M); ++i){
                sum += ChebyPoly(N - 1, x0 * std::cos(pi * i / N)) * std::cos(2.0f * n * pi * i / N);
            }
            x[nn] = tg + 2*sum;
            x[N - nn - 1] = x[nn];
        }
    }

    static void ApplyWindow(std::span<float> x, float atten) {
        const size_t N = x.size();
        float tg = std::pow(10,atten/20);
        float x0 = std::cosh((1.0 / (N - 1)) * std::acosh(tg));
        float M = (N - 1.0f) / 2.0f;
        if(N % 2 == 0) {
            M = M + 0.5;
        }
        constexpr float pi = std::numbers::pi_v<float>;
        for(size_t nn = 0; nn < (N / 2 + 1); ++nn){
            float n = nn - M;
            float sum = 0;
            for(size_t i = 1; i <= static_cast<size_t>(M); ++i){
                sum += ChebyPoly(N - 1, x0 * std::cos(pi * i / N)) * std::cos(2.0f * n * pi * i / N);
            }
            float wx = tg + 2 * sum;
            x[nn] *= wx;
            x[N - nn - 1] *= wx;
        }
    }

    [[deprecated("not implement")]]
    static void DWindow(std::span<float> x) {
        
    }

    static float ChebyPoly(int n, float x){
        if (std::abs(x) <= 1.0f) {
            return std::cos(n * std::acos(x));
        }
        else {
            return std::cosh(n * std::acosh(x));
        }
    }
};
}