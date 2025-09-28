#pragma once
#include <span>
#include <vector>

namespace qwqdsp::adaptive {
class BurgLP {
public:
    void Init(size_t block_len) {
        eb_.resize(block_len);
    }

    /**
     *                     +-----k----+
     *                     |          ↓
     *   x -------------------------> + -----> x
     *                     |    |
     *                     |    +--k-+
     *            +-----+  |         ↓
     *  eb -------|z^-1|-----------> + ------> eb
     *            +----+
     */
    void Process(std::span<float> x, std::span<float> latticek) noexcept {
        // std::copy(x.begin(), x.end(), eb_.begin());
        // for (auto& k : latticek) {
        //     float lag{};
        //     float up{};
        //     float down{};
        //     for (size_t i = 0; i < x.size(); ++i) {
        //         up += x[i] * lag;
        //         down += x[i] * x[i];
        //         down += lag * lag;
        //         lag = eb_[i];
        //     }
        //     k = -2.0f * up / down;

        //     lag = 0;
        //     for (size_t i = 0; i < x.size(); ++i) {
        //         float const upgo = x[i] + lag * k;
        //         float const downgo = lag + x[i] * k;
        //         lag = eb_[i];
        //         x[i] = upgo;
        //         eb_[i] = downgo;
        //     }
        // }

        std::copy(x.begin() + 1, x.end(), eb_.begin());
        for (size_t kidx = 0; auto& k : latticek) {
            ++kidx;

            float up{};
            float down{};
            for (size_t i = 0; i < x.size() - kidx; ++i) {
                up += x[i] * eb_[i];
                down += x[i] * x[i];
                down += eb_[i] * eb_[i];
            }
            k = -2.0f * up / down;

            for (size_t i = 0; i < x.size() - kidx - 1; ++i) {
                float const upgo = x[i] + eb_[i] * k;
                float const downgo = eb_[i + 1] + x[i + 1] * k;
                x[i] = upgo;
                eb_[i] = downgo;
            }
        }
    }

    /**
     * @note 执行之后k会被b复写, b.size() = k.size()
     * @param b sum b[i] * z^-i, i from 1 to k.size
     */
    static void Lattice2Tf(std::span<float> k, std::span<float> b) noexcept {
        for (size_t i = 0; i < k.size(); ++i) {
            for(size_t j = 0; j+1 <= i; j++) {
                k[j] = b[j] + k[i] * b[i - j - 1];
            }

            for(size_t j = 0; j <= i; j++) {
                b[j] = k[j];
            }
        }
    }

    /**
     * @note upgoing.size() = downgoing.size() = k.size() + 1
     * @param upgoing sum upgoing[i] * z^-i, i from 0 to k.size, 最小相位
     * @param downgoing sum downgoing[i] * z^-i, i from 0 to k.size, 最大相位
     */
    static void Lattice2Tf_KeepK(std::span<const float> k, std::span<float> upgoing, std::span<float> downgoing) noexcept {
        for (size_t kidx = 0; kidx < k.size(); ++kidx) {
            for (size_t i = kidx + 1; i != 0; --i) {
                downgoing[i] = downgoing[i - 1];
            }
            downgoing[0] = 0;

            for (size_t i = 0; i < kidx + 2; ++i) {
                float up = upgoing[i] + k[kidx] * downgoing[i];
                float down = downgoing[i] + k[kidx] * upgoing[i];
                upgoing[i] = up;
                downgoing[i] = down;
            }
        }
    }
private:
    std::vector<float> eb_;
};
}