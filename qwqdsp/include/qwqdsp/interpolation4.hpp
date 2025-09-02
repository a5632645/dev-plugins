#pragma once
#include <cmath>

namespace qwqdsp {
/**
* @brief Interpolation Full
*/
struct Interpolation4 {
    static float Lagrange3rd(
        float y0, float y1, float y2, float y3,
        float x0, float x1, float x2, float x3,
        float x
    ) noexcept {
        auto f1 = (x - x1) * (x - x2) * (x - x3) / (x0 - x1) / (x0 - x2) / (x0 - x3);
        auto f2 = (x - x0) * (x - x2) * (x - x3) / (x1 - x0) / (x1 - x2) / (x1 - x3);
        auto f3 = (x - x0) * (x - x1) * (x - x3) / (x2 - x0) / (x2 - x1) / (x2 - x3);
        auto f4 = (x - x0) * (x - x1) * (x - x2) / (x3 - x0) / (x3 - x1) / (x3 - x2);
        return f1 * y0 + f2 * y1 + f3 * y2 + f4 * y3;
    }

    /**
     * @ref https://blog.csdn.net/qq_33552519/article/details/102742715
     */
    static float SPPCHIP(
        float y0, float y1, float y2, float y3,
        float x0, float x1, float x2, float x3,
        float x
    ) noexcept {
        auto e0 = (y1 - y0) / (x1 - x0);
        auto e1 = (y2 - y1) / (x2 - x1);
        auto e2 = (y3 - y2) / (x3 - x2);

        auto h0 = x1 - x0;
        auto h1 = x2 - x1;
        auto h2 = x3 - x2;

        auto d0 = ((2 * h0 + h1) * e0 - h0 * e1) / (h0 + h1);
        if (d0 * e0 < 0) {
            d0 = 0;
        }
        else if (e0 * e1 < 0 && std::abs(d0) > 3.0f * std::abs(e0)) {
            d0 = 3.0f * e0;
        }

        auto d3 = ((2 * h2 + h1) * e2 - h2 * e1) / (h1 + h2);
        if (d3 * e2 < 0) {
            d3 = 0;
        }
        else if (e2 * e1 < 0 && std::abs(d3) > 3.0f * std::abs(e2)) {
            d3 = 3.0f * e2;
        }

        auto d1 = 0.0f;
        if (e0 == 0.0f || e1 == 0.0f || e0 * e1 < 0) {
            d1 = 0;
        }
        else {
            auto w1 = 2 * h1 + h0;
            auto w2 = h1 + 2 * h0;
            d1 = (w1 + w2) * (e0 * e1) / (w1 * e1 + w2 * e0);
        }

        auto d2 = 0.0f;
        if (e1 == 0.0f || e2 == 0.0f || e1 * e2 < 0) {
            d2 = 0;
        }
        else {
            auto w1 = 2 * h2 + h1;
            auto w2 = h2 + 2 * h1;
            d2 = (w1 + w2) * (e2 * e1) / (w1 * e2 + w2 * e1);
        }

        if (x < x1) {
            auto s = x - x0;
            auto c0 = (3 * e0 - 2 * d0 - d1) / h0;
            auto b0 = (d0 - 2 * e0 + d1) / (h0 * h0);
            return y0 + s * d0 + s * s * c0 + s * s * s * b0;
        }
        else if (x > x2) {
            auto s = x - x2;
            auto c0 = (3 * e2 - 2 * d2 - d3) / h2;
            auto b0 = (d2 - 2 * e2 + d3) / (h2 * h2);
            return y2 + s * d2 + s * s * c0 + s * s * s * b0;
        }
        else {
            auto s = x - x1;
            auto c0 = (3 * e1 - 2 * d1 - d2) / h1;
            auto b0 = (d1 - 2 * e1 + d2) / (h1 * h1);
            return y1 + s * d1 + s * s * c0 + s * s * s * b0;
        }
    }

    static float Spline(
        float y0, float y1, float y2, float y3,
        float x0, float x1, float x2, float x3,
        float x
    ) noexcept {
        auto m0 = (y1 - y0) / (x1 - x0);
        auto m1 = (y2 - y1) / (x2 - x1);
        auto m2 = (y3 - y2) / (x3 - x2);
        auto z2 = (m2 * x1 + m1 * x2 - m2 * x2 + 2 * m1 * x3 + 2 * m0 * x1 - 2 * m0 * x3 - 3 * m1 * x1) / (4 * (x0 * x1 + x2 * x3 - x0 * x3) - (x1 + x2) * (x1 + x2));
        auto z3 = (m1 * x1 + m0 * x2 - m0 * x1 + 2 * m1 * x0 + 2 * m2 * x2 - 2 * m2 * x0 - 3 * m1 * x2) / (4 * (x0 * x1 + x2 * x3 - x0 * x3) - (x1 + x2) * (x1 + x2));
        auto a1 = z2 / (x0 - x1);
        auto b1 = 2 * z2 / (x1 - x0);
        auto a2 = (2 * z2 + z3) / (x1 - x2);
        auto b2 = (2 * z3 + z2) / (x2 - x1);
        auto a3 = 2 * z3 / (x2 - x3);
        auto b3 = z3 / (x3 - x2);
        if (x < x1) {
            auto s1 = x - x1;
            auto s2 = x - x0;
            auto c = a1 * s1 * s1 * s2 + b1 * s1 * s2 * s2;
            auto l = m0 * s2 + y0;
            return l + c;
        }
        else if (x < x2) {
            auto s1 = x - x2;
            auto s2 = x - x1;
            auto c = a2 * s1 * s1 * s2 + b2 * s1 * s2 * s2;
            auto l = m1 * s2 + y1;
            return l + c;
        }
        else {
            auto s1 = x - x3;
            auto s2 = x - x2;
            auto c = a3 * s1 * s1 * s2 + b3 * s1 * s2 * s2;
            auto l = m2 * s1 + y3;
            return l + c;
        }
    }

    static float CatmullRomSpline(
        float y0, float y1, float y2, float y3,
        float x0, float x1, float x2, float x3,
        float x, float tension
    ) noexcept {
        tension = 1.0f - tension;
        float t01 = std::sqrt(x1 - x0);
        float t12 = std::sqrt(x2 - x1);
        float t23 = std::sqrt(x3 - x2);
        float m1 = (1.0f - tension) * (y2 - y1 + t12 * ((y1 - y0) / t01 - (y2 - y0) / (t01 + t12)));
        float m2 = (1.0f - tension) * (y2 - y1 + t12 * ((y3 - y2) / t23 - (y3 - y1) / (t12 + t23)));
        if (x < x1) {
            // auto m0 = m1;
            auto m0 = (y1 - y0) / (x1 - x0);
            auto a = 2.0f * (y0 - y1) + m0 + m1;
            auto b = -3.0f * (y0 - y1) - 2.0f * m0 - m1;
            auto c = m0;
            auto d = y0;
            auto s = (x - x0) / (x1 - x0);
            return d + s * (
                c + s * (b
                    + s * a
                )
            );
        }
        else if (x > x2) {
            // float m3 = m2;
            float m3 = (y3 - y2) / (x3 - x2);
            auto a = 2.0f * (y2 - y3) + m2 + m3;
            auto b = -3.0f * (y2 - y3) - 2.0f * m2 - m3;
            auto c = m2;
            auto d = y2;
            auto s = (x - x2) / (x3 - x2);
            return d + s * (
                c + s * (b
                    + s * a
                )
            );
        }
        else {
            auto a = 2.0f * (y1 - y2) + m1 + m2;
            auto b = -3.0f * (y1 - y2) - 2.0f * m1 - m2;
            auto c = m1;
            auto d = y1;
            auto s = (x - x1) / (x2 - x1);
            return d + s * (
                c + s * (b
                    + s * a
                )
            );
        }
    }

    /**
     * @ref https://blogs.mathworks.com/cleve/2019/04/29/makima-piecewise-cubic-interpolation/?from=cn
     */
    static float Makima(
        float yn2, float yn1, float y0, float y1, float y2, float y3,
        float xn2, float xn1, float x0, float x1, float x2, float x3,
        float x
    ) noexcept {
        float en2 = (yn1 - yn2) / (xn2 - xn1);
        float en1 = (y0 - yn1) / (x0 - xn1);
        float e0 = (y1 - y0) / (x1 - x0);
        float e1 = (y2 - y1) / (x2 - x1);
        float e2 = (y3 - y2) / (x3 - x2);
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
        float s = x - x0;
        float h0 = x1 - x0;
        float c0 = (3 * e0 - 2 * d0 - d1) / h0;
        float b0 = (d0 - 2 * e0 + d1) / (h0 * h0);
        return y0 + s * d0 + s * s * c0 + s * s * s * b0;
    }
};
}