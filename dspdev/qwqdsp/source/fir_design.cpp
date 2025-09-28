#include "qwqdsp/filter/fir_design.hpp"

#include <cassert>
#include <numbers>
#include <cmath>
#include <complex>
#include <Eigen/Dense>

static inline double Sinc(double x) {
    [[unlikely]]
    if (x == 0.0) {
        return 1.0;
    }
    else {
        return std::sin (x * std::numbers::pi) / (std::numbers::pi * x);
    }
}

namespace qwqdsp::filter {
// from juce::dsp::FilterDesign
void FirDesign::LowpassLeastSquare(
    std::span<float> x,
    float cutoff,
    float transist_width, float stop_band_weight
) {
    
    double wpass = cutoff - transist_width / 2;
    double wstop = cutoff + transist_width / 2;
    int N = static_cast<int>(x.size());

    if (N % 2 == 1)
    {
        // Type I
        auto M = (N - 1) / 2;

        Eigen::VectorX<double> b (M + 1);
        Eigen::VectorX<double> q (2 * M + 1);

        auto factorp = wpass / std::numbers::pi;
        auto factors = wstop / std::numbers::pi;

        for (int i = 0; i <= M; ++i) {
            b[i] = factorp * Sinc(factorp * i);
        }

        q[0] = factorp + stop_band_weight * (1.0 - factors);
        for (int i = 1; i <= 2 * M; ++i) {
            q[i] = factorp * Sinc(factorp * i) - stop_band_weight * factors * Sinc(factors * i);
        }

        // toeplitz
        Eigen::MatrixX<double> Q(M + 1, M + 1);
        for (int i = 0; i < M + 1; ++i) {
            Q(i, i) =  0.5 * q[0];
        }
        for (int i = 1; i < M + 1; ++i) {
            for (int j = i; j < M + 1; ++j) {
                Q(j, j - i) = 0.5 * q[i];
                Q(j - i, j) = 0.5 * q[i];
            }
        }

        // hankel
        for (int i = 0; i < M + 1; ++i) {
            Q(i, i) += 0.5 * q[2 * i];
        }
        for (int i = 1; i < M + 1; ++i) {
            for (int j = i; j < M + 1; ++j) {
                Q(j, j - i) += 0.5 * q[i + 2 * (j - i)];
                Q(j - i, j) += 0.5 * q[i + 2 * (j - i)];
            }
        }

        auto qr = Q.colPivHouseholderQr();
        auto h = qr.solve(b);

        x[M] = static_cast<float>(h[0]);
        for (int i = 1; i <= M; ++i) {
            x[M - i] = static_cast<float>(h(i) * 0.5);
            x[M + i] = static_cast<float>(h(i) * 0.5);
        }
    }
    else
    {
        // Type II
        auto M = N / 2;

        Eigen::VectorX<double> b (M);
        Eigen::VectorX<double> qp (2 * M);
        Eigen::VectorX<double> qs (2 * M);

        auto factorp = wpass / std::numbers::pi;
        auto factors = wstop / std::numbers::pi;

        for (int i = 0; i < M; ++i) {
            b[i] = factorp * Sinc(factorp * (i + 0.5));
        }

        for (int i = 0; i < 2 * M; ++i) {
            qp[i] = 0.25 * factorp * Sinc(factorp * i);
            qs[i] = -0.25 * stop_band_weight * factors * Sinc(factors * i);
        }

        Eigen::MatrixX<double> Q(M, M);
        Q.setIdentity();
        Q *= (0.25 * stop_band_weight);
        // toeplitz
        for (int i = 0; i < M; ++i) {
            Q(i, i) += qp[0];
            Q(i, i) += qs[0];
        }
        for (int i = 1; i < M; ++i) {
            for (int j = i; j < M; ++j) {
                Q(j, j - i) += qp[i];
                Q(j, j - i) += qs[i];
                Q(j - i, j) += qp[i];
                Q(j - i, j) += qs[i];
            }
        }

        // hankel
        for (int i = 0; i < M; ++i) {
            Q(i, i) += qp[2 * i + 1];
            Q(i, i) += qs[2 * i + 1];
        }
        for (int i = 1; i < M; ++i) {
            for (int j = i; j < M; ++j) {
                Q(j, j - i) += qp[i + 1 + 2 * (j - i)];
                Q(j - i, j) += qp[i + 1 + 2 * (j - i)];
                Q(j, j - i) += qs[i + 1 + 2 * (j - i)];
                Q(j - i, j) += qs[i + 1 + 2 * (j - i)];
            }
        }

        auto qr = Q.colPivHouseholderQr();
        auto h = qr.solve(b);

        for (int i = 0; i < M; ++i) {
            x[M - i - 1] = static_cast<float> (h[i] * 0.25);
            x[M + i]     = static_cast<float> (h[i] * 0.25);
        }
    }
}

void FirDesign::GainResponce(std::span<float> coeffs, std::span<float> gains) {
    for (size_t i = 0; i < gains.size(); ++i) {
        auto w = static_cast<float>(i) * std::numbers::pi_v<float> / static_cast<float>(gains.size());
        auto z = std::polar(1.0f, -w);
        std::complex<float> e = 1;
        std::complex<float> sum = 0;
        for (auto& h : coeffs) {
            sum += h * e;
            e *= z;
        }
        gains[i] = std::abs(sum);
    }
}

}