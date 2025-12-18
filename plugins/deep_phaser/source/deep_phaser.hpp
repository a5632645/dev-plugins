#pragma once
#include "shared.hpp"

#include <qwqdsp/misc/smoother.hpp>
#include <qwqdsp/spectral/complex_fft.hpp>
#include <qwqdsp/oscillator/vic_sine_osc.hpp>
#include <qwqdsp/extension_marcos.hpp>
#include <qwqdsp/convert.hpp>
#include <qwqdsp/simd_element/simd_element.hpp>

using SimdType = qwqdsp_simd_element::PackFloat<4>;
using SimdIntType = qwqdsp_simd_element::PackInt32<4>;

class AllpassBuffer2 {
public:
    static constexpr size_t kRealNumApf = 512;
    static constexpr size_t kNumApf = kRealNumApf / SimdType::kSize;
    static constexpr size_t kIndexSize = kNumApf * SimdType::kSize;
    static constexpr size_t kIndexMask = kNumApf * SimdType::kSize - 1;
    static constexpr float kMaxIndex = kIndexSize;

    void Reset() noexcept {
        std::ranges::fill(output_buffer_, SimdType{});
        std::ranges::fill(xlags_, SimdType{});
    }

    QWQDSP_FORCE_INLINE
    auto GetAfterPush(SimdIntType rpos) const noexcept {
        SimdIntType mask = SimdIntType::vBroadcast(kIndexMask);
        SimdIntType irpos = rpos & mask;

        struct LRSimdType {
            SimdType left;
            SimdType right;
        };

        SimdType const* buffer = xlags_.data();
        SimdType y0;
        y0[0] = buffer[irpos[0]][0];
        y0[1] = buffer[irpos[1]][0];
        y0[2] = buffer[irpos[2]][0];
        y0[3] = buffer[irpos[3]][0];
        SimdType y1;
        y1[0] = buffer[irpos[0]][1];
        y1[1] = buffer[irpos[1]][1];
        y1[2] = buffer[irpos[2]][1];
        y1[3] = buffer[irpos[3]][1];
        return LRSimdType{y0,y1};
    }

    QWQDSP_FORCE_INLINE
    void Push(float left_x, float rightx, float left_coeff, float right_coeff, size_t num_cascade) noexcept {
        SimdType* xlag_ptr = xlags_.data();
        SimdType* ylag_ptr = output_buffer_.data();
        SimdType coeff{left_coeff, right_coeff};
        SimdType x{left_x, rightx};
        for (size_t i = 0; i < num_cascade; ++i) {
            SimdType yout = *xlag_ptr + coeff * (x - *ylag_ptr);
            *xlag_ptr = x;
            *ylag_ptr = yout;
            x = yout;
            ++xlag_ptr;
            ++ylag_ptr;
            // float left_y = xlag_ptr[0] + left_coeff * (left_x - ylag_ptr[0]);
            // float righty = xlag_ptr[1] + right_coeff * (rightx - ylag_ptr[1]);
            // xlag_ptr[0] = left_x;
            // xlag_ptr[1] = rightx;
            // ylag_ptr[0] = left_y;
            // ylag_ptr[1] = righty;
            // left_x = left_y;
            // rightx = righty;
            // xlag_ptr += 2;
            // ylag_ptr += 2;
        }
    }

    void SetZero(size_t last_num, size_t curr_num) noexcept {
        size_t const begin_idx = last_num;
        size_t const end_idx = curr_num;
        for (size_t i = begin_idx; i < end_idx; ++i) {
            output_buffer_[i] = {};
        }
        for (size_t i = begin_idx; i < end_idx; ++i) {
            xlags_[i] = {};
        }
    }

    /**
     * @note won't check w
     * @param w [0~pi]
     */
    static float ComputeCoeff(float w) noexcept {
        auto k = std::tan(w / 2);
        return (k - 1) / (k + 1);
    }

    static float RevertCoeff2Omega(float coeff) noexcept {
        return std::atan((1 - coeff) / (1 + coeff));
    }

    SimdType GetLR(size_t filter_idx) const noexcept {
        // return SimdType{xlags_[filter_idx * 2], xlags_[filter_idx * 2 + 1]};
        return xlags_[filter_idx];
    }
private:
    alignas(16) std::array<SimdType, kRealNumApf> output_buffer_{};
    alignas(16) std::array<SimdType, kRealNumApf> xlags_{};
};
