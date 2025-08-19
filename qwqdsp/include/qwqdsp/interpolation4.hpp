#pragma once

namespace qwqdsp {
/**
* @brief Interpolation Full
*/
struct Interpolation4 {
    static float Lagrange3rd(
        float y0, float y1, float y2, float y3,
        float x0, float x1, float x2, float x3,
        float x
    ) {
        auto f1 = (x - x1) * (x - x2) * (x - x3) / (x0 - x1) / (x0 - x2) / (x0 - x3);
        auto f2 = (x - x0) * (x - x2) * (x - x3) / (x1 - x0) / (x1 - x2) / (x1 - x3);
        auto f3 = (x - x0) * (x - x1) * (x - x3) / (x2 - x0) / (x2 - x1) / (x2 - x3);
        auto f4 = (x - x0) * (x - x1) * (x - x2) / (x3 - x0) / (x3 - x1) / (x3 - x2);
        return f1 * y0 + f2 * y1 + f3 * y2 + f4 * y3;
    }

    // TODO: fix it
    [[deprecated("not implement")]]
    static float SPPCHIP(
        float y0, float y1, float y2, float y3,
        float x0, float x1, float x2, float x3,
        float x
    ) {
        auto e0 = (y1 - y0) / (x1 - x0);
        auto e1 = (y2 - y1) / (x2 - x1);
        auto e2 = (y3 - y2) / (x3 - x2);

        auto h0 = x1 - x0;
        auto h1 = x2 - x1;
        auto h2 = x3 - x2;

        auto d0 = ((2 * h0 + h1) * e0 - h0 * e2) / (h0 + h1);
        auto d3 = ((2 * h2 + h1) * e2 - h2 * e1) / (h1 + h2);
        auto d1 = 0.0f;
        if (e0 == 0.0f || e1 == 0.0f) {
            d1 = 0;
        }
        else {
            auto w1 = 2 * h1 + h0;
            auto w2 = h1 + 2 * h0;
            d1 = (w1 + w2) * (e0 * e1) / (w1 * e1 + w2 * e0);
        }
        auto d2 = 0.0f;
        if (e1 == 0.0f || e2 == 0.0f) {
            d2 = 0;
        }
        else {
            auto w1 = 2 * h2 + h1;
            auto w2 = h2 + 2 * h1;
            d2 = (w1 + w2) * (e2 * e1) / (w1 * e2 + w2 * e1);
        }

        if (x < x1) {
            auto s = x - x0;
            auto c0 = (3 * e0 - 2 * d0 - d1) / e0;
            auto b0 = (d0 - 2 * e0 + d1) / (e0 * e0);
            return y0 + s * d0 + s * s * c0 + s * s * s * b0;
        }
        else if (x > x2) {
            auto s = x - x2;
            auto c0 = (3 * e2 - 2 * d2 - d3) / e2;
            auto b0 = (d2 - 2 * e2 + d3) / (e2 * e2);
            return y2 + s * d2 + s * s * c0 + s * s * s * b0;
        }
        else {
            auto s = x - x1;
            auto c0 = (3 * e1 - 2 * d1 - d2) / e1;
            auto b0 = (d1 - 2 * e1 + d2) / (e1 * e1);
            return y1 + s * d1 + s * s * c0 + s * s * s * b0;
        }
    }

    // TODO: fix it
    [[deprecated("not implement")]]
    static float Spline(
        float y0, float y1, float y2, float y3,
        float x0, float x1, float x2, float x3,
        float x
    ) {
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
        else if (x > x2) {
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

    // TODO: fix it
    [[deprecated("not implement")]]
    static float CatmullRomSpline(
        float y0, float y1, float y2, float y3,
        float x0, float x1, float x2, float x3,
        float x, float tension
    ) {
        tension = 1.0f - tension;
        auto q1 = (y1 - y0) / (x1 - x0) - (y2 - y0) / (x2 - x0) + (y2 - y1) / (x2 - x1);
        auto q2 = (y2 - y1) / (x2 - x1) - (y3 - y1) / (x3 - x1) + (y3 - y2) / (x3 - x2);
        auto m1 = tension * (x2 - x1) * q1;
        auto m2 = tension * (x2 - x1) * q2;
        auto a = 2 * y1 - 2 * y2 + m1 + m2;
        auto b = -3 * y1 + 3 * y2 - 2 * m1 - m2;
        auto c = m1;
        auto d = y1;
        if (x < x1) {

        }
        else if (x > x2) {

        }
        else {
            auto s = x - x1;
            return d + s * (
                c + s * (b
                    + s * a
                )
            );
        }
    }

    // TODO: fix it
    [[deprecated("not implement")]]
    static float Makima(
        float yn2, float yn1, float y0, float y1, float y2,
        float xn2, float xn1, float x0, float x1, float x2,
        float x
    ) {
        
    }
};
}