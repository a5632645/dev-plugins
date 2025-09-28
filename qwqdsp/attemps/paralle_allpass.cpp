class ParalleAllpass {
public:
    void Reset() noexcept {
        lag_ = 0;
        lag1_.fill(0);
        lag2_.fill(0);
    }

    void SetPoles(std::array<float, 4> radius, std::array<float, 4> w) noexcept {
        std::array<std::complex<double>, 4> poles;
        size_t n = 0;
        for (size_t i = 0; i < 4; ++i) {
            auto p = std::polar<double>(radius[i], w[i]);
            bool too_close = false;
            for (size_t j = 0; j < n; ++j) {
                auto const distance = std::abs(poles[j] - p);
                if (distance < 1e-2f) {
                    too_close = true;
                    break;
                }
            }

            if (!too_close) {
                poles[n++] = p;
            }
        }

        double f0 = 1;
        for (size_t i = 0; i < n; ++i) {
            f0 *= std::norm(poles[i]);
        }
        f0_ = f0;

        for (size_t i = 0; i < n; ++i) {
            auto const z = poles[i];
            std::complex<double> r = 1;
            for (size_t j = 0; j < n; ++j) {
                auto const p1 = poles[j];
                auto const p2 = std::conj(p1);
                if (i == j) {
                    r *= (p1*z-1.0)/(z-p2)*(p2*z-1.0);
                }
                else {
                    r *= (p1*z-1.0)/(z-p2)*(p2*z-1.0)/(z-p1);
                }
            }
            c1_[i] = 2*r.real();
            c2_[i] = 2*(z.real()*r.real()+z.imag()*r.imag());
            a1_[i] = -2*z.real();
            a2_[i] = std::norm(z);
        }

        for (size_t i = n; i < 4; ++i) {
            c1_[i] = 0;
            c2_[i] = 0;
            a1_[i] = 0;
            a2_[i] = 0;
            lag1_[i] = 0;
            lag2_[i] = 0;
        }
    }

    float Tick(float const x) noexcept {
        float y = f0_ * x;
        for (size_t i = 0; i < 4; ++i) {
            float t = lag_ - a1_[i] * lag1_[i] - a2_[i] * lag2_[i];
            float output = t * c1_[i] - c2_[i] * lag1_[i];
            lag2_[i] = lag1_[i];
            lag1_[i] = t;
            y += output;
        }
        lag_ = x;
        return y;
    }
private:
    std::array<float, 4> c1_{};
    std::array<float, 4> c2_{};
    std::array<float, 4> a1_{};
    std::array<float, 4> a2_{};
    std::array<float, 4> lag1_{};
    std::array<float, 4> lag2_{};
    float f0_{};
    float lag_{};
};