#pragma once
#include <cmath>

namespace dsp {

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
    ) {
        auto d1 = frac - 1.f;
        auto d2 = frac - 2.f;
        auto d3 = frac - 3.f;

        auto c1 = -d1 * d2 * d3 / 6.f;
        auto c2 = d2 * d3 * 0.5f;
        auto c3 = -d1 * d3 * 0.5f;
        auto c4 = d1 * d2 / 6.f;

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
    ) {
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

    // TODO: fix these
    // static float Spline(
    //     float yn1, float y0, float y1, float y2,
    //     float frac
    // ) {
    //     auto d0 = (y1 - yn1) / 2.0f;
    //     auto d1 = (y2 - y0) / 2.0f;
    //     auto d = y1 - y0;
    //     auto t = frac * frac * (3.0f - 2.0f * frac);
    //     return (2.0f * y0 - 2.0f * y1 + d0 + d1) * t + d * frac;
    // }

    // static float Akima(
    //     float yn2, float yn1, float y0, float y1, float y2, float frac
    // ) {
    //     auto m0 = (y0 - yn2) * 0.5f;
    //     auto m1 = (y1 - yn1) * 0.5f;
    //     auto m2 = (y2 - y0) * 0.5f;

    //     auto w1 = std::abs(y1 - yn1);
    //     auto w2 = std::abs(y2 - y0);
    //     auto w3 = std::abs(y0 - yn2);

    //     auto s1 = w1 + w3 != 0.f ? (w1 * m2 + w3 * m1) / (w1 + w3) : 0.f;
    //     auto s2 = w1 + w2 != 0.f ? (w1 * m2 + w2 * m1) / (w1 + w2) : 0.f;

    //     auto a = 2.0f * (y0 - y1) + s1 + s2;
    //     auto b = 3.0f * (y1 - y0) - 2.0f * s1 - s2;
    //     auto c = s1;
    //     auto d = y0;

    //     return a * frac * frac * frac + b * frac * frac + c * frac + d;
    // }

    // static float Makima(
    //     float yn2, float yn1, float y0, float y1, float y2, float frac
    // ) {
    //     auto m0 = (y0 - yn2) * 0.5f;
    //     auto m1 = (y1 - yn1) * 0.5f;
    //     auto m2 = (y2 - y0) * 0.5f;

    //     auto w1 = std::abs(y1 - yn1);
    //     auto w2 = std::abs(y2 - y0);
    //     auto w3 = std::abs(y0 - yn2);
    //     auto w4 = std::abs(y2 - yn1);

    //     auto s1 = w1 + w3 != 0.f ? (w1 * m2 + w3 * m1) / (w1 + w3) : 0.f;
    //     auto s2 = w1 + w2 != 0.f ? (w1 * m2 + w2 * m1) / (w1 + w2) : 0.f;
    //     auto s3 = w2 + w4 != 0.f ? (w2 * m1 + w4 * m0) / (w2 + w4) : 0.f;
    //     auto s4 = w3 + w4 != 0.f ? (w3 * m1 + w4 * m0) / (w3 + w4) : 0.f;

    //     auto a = 2.0f * (y0 - y1) + s1 + s2;
    //     auto b = 3.0f * (y1 - y0) - 2.0f * s1 - s2;
    //     auto c = s1 - s3;
    //     auto d = s4 - s1;

    //     return a * frac * frac * frac + b * frac * frac + c * frac + d;
    // }
};

}