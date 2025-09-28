#pragma once
#include <cmath>

namespace qwqdsp {

struct Interpolation {
    /**
     * @brief 拉格朗日三次插值
     * @note  在非0和1的情况下在高频的振幅响应会超过1，不适合用于有反馈的系统
     *
     * @param y0   y[x=0]
     * @param y1   y[x=1]
     * @param y2   y[x=2]
     * @param y3   y[x=3]
     * @param frac 0<x<1
     *
     */
    static float Lagrange3rd(
        float y0, float y1, float y2, float y3,
        float frac
    ) noexcept {
        auto d1 = frac - 1.0f;
        auto d2 = frac - 2.0f;
        auto d3 = frac - 3.0f;

        auto c1 = -d1 * d2 * d3 / 6.0f;
        auto c2 = d2 * d3 * 0.5f;
        auto c3 = -d1 * d3 * 0.5f;
        auto c4 = d1 * d2 / 6.0f;

        return y0 * c1 + frac * (y1 * c2 + y2 * c3 + y3 * c4);
    }

    /**
    * @brief 分段三次 Hermite 插值
    * @note  无单调性保证，越接近0.5高频损失越多
    * @param yn1  y[x=-1]
    * @param y0   y[x=0]
    * @param y1   y[x=1]
    * @param y2   y[x=2]
    * @param frac 0<x<1
    */
    static float PCHIP(
        float yn1, float y0, float y1, float y2,
        float frac
    ) noexcept {
        auto d0 = (y1 - yn1) / 2.0f;
        auto d1 = (y2 - y0) / 2.0f;
        auto d = y1 - y0;
        return y0 + frac * (
            d0 + frac * (
                3.0f * d - 2.0f * d0 - d1 + frac * (
                    d0 - 2.0f * d + d1
                )
            )
        );
    }

    /**
     * @brief  Cubic Spline
     * @note   在反馈下稳定
     * @ref    https://www.desmos.com/calculator/hycpcbwtbq?lang=zh-CN
     *
     * @param yn1  y[x=-1]
     * @param y0   y[x=0]
     * @param y1   y[x=1]
     * @param y2   y[x=2]
     * @param frac 0<x<1
     */
    static float Spline(
        float yn1, float y0, float y1, float y2,
        float frac
    ) noexcept {
        auto m1 = y0 - yn1;
        auto m2 = y1 - y0;
        auto m3 = y2 - y1;
        auto z2 = (-m3 + 5 * m2 - 4 * m1) / 15.0f;
        auto z3 = (m1 - 5 * m2 + 4 * m3) / 15.0f;
        auto a2 = -(2 * z2 + z3);
        auto b2 = 2 * z3 + z2;
        auto l2 = y0 + m2 * frac;
        auto t = frac - 1;
        auto c2 = t * frac * (a2 * t + b2 * frac);
        return l2 + c2;
    }

    /**
     * @brief  Catmull-ROM Spline
     * @note   在反馈下稳定
     * @ref    https://qroph.github.io/2018/07/30/smooth-paths-using-catmull-rom-splines.html
     *
     * @param yn1  y[x=-1]
     * @param y0   y[x=0]
     * @param y1   y[x=1]
     * @param y2   y[x=2]
     * @param frac 0<x<1
     * @tparam tension 0<=x<=1
     */
    template<float tension = 0.0f>
    static float CatmullRomSpline(
        float yn1, float y0, float y1, float y2,
        float frac
    ) noexcept {
        auto m0 = (1.0f - tension) * 0.5f * (y1 - yn1);
        auto m1 = (1.0f - tension) * 0.5f * (y2 - y0);

        auto a = 2.0f * y0 - 2.0f * y1 + m0 + m1;
        auto b = -3.0f * y0 + 3.0f * y1 - 2.0f * m0 - m1;
        auto c = m0;
        auto d = y0;

        return d + frac * (
            c + frac * (
                b + a * frac
            )
        );
    }

    static float Linear(
        float y0, float y1,
        float frac
    ) noexcept {
        return y0 + frac * (y1 - y0);
    }

    /**
     * @ref https://blogs.mathworks.com/cleve/2019/04/29/makima-piecewise-cubic-interpolation/?from=cn
     */
    static float Makima(
        float yn2, float yn1, float y0, float y1, float y2, float y3,
        float frac
    ) noexcept {
        float en2 = yn1 - yn2;
        float en1 = y0 - yn1;
        float e0 = y1 - y0;
        float e1 = y2 - y1;
        float e2 = y3 - y2;
        float w1 = std::abs(e1 - e0) + std::abs(e1 + e0) * 0.5f;
        float w2 = std::abs(en1 - en2) + std::abs(en1 + en2) * 0.5f;
        float w1x = std::abs(e2 - e1) + std::abs(e2 + e1) * 0.5f;
        float w2x = std::abs(e0 - en1) + std::abs(e0 + en1) * 0.5f;
        float d0 = (w1 * en1 + w2 * e0) / (w1 + w2);
        float d1 = (w1x * e0 + w2x * e1) / (w1x + w2x);
        [[unlikely]]
        if (std::isnan(d0)) {
            d0 = 0.0f;
        }
        [[unlikely]]
        if (std::isnan(d1)) {
            d1 = 0.0f;
        }
        float d = y1 - y0;
        return y0 + frac * (
            d0 + frac * (
                3.0f * d - 2.0f * d0 - d1 + frac * (
                    d0 - 2.0f * d + d1
                )
            )
        );
    }
};

}