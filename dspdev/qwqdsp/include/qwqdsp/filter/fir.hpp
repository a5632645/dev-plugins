#pragma once
#include <algorithm>
#include <cstddef>
#include <span>
#include <vector>
#include "qwqdsp/segement/slice.hpp"

namespace qwqdsp::filter {
/**
 * @tparam kBatchSize 最大一次性处理多少采样，每隔这个数量才会执行一次移动采样，越大平均移动越少，同时内存增加
 */
template <size_t kBatchSize>
class FIRDirect {
public:
    void Reset() noexcept {
        std::fill(latch_.begin(), latch_.end(), 0.0f);
    }

    /**
     * @brief 滤波器的参数是h(0)...h(n-1)排列，且coeff的大小需要手动分配
     * @tparam Func void(std::vector<float>& coeff)
     */
    template <class Func>
    void SetCoeff(Func&& func) {
        func(coeff_);
        std::reverse(coeff_.begin(), coeff_.end());
        size_t const size = coeff_.size() + kBatchSize - 1;
        size_t const old_size = latch_.size();
        if (old_size < size) {
            latch_.resize(size);
            for (size_t i = 0; i < old_size; ++i) {
                size_t const target = size - i - 1;
                size_t const source = old_size - i - 1;
                latch_[target] = latch_[source];
            }
            std::fill_n(latch_.begin(), size - old_size, 0.0f);
        }
    }

    void Process(std::span<float> x) noexcept {
        segement::Slice1D slice{x};
        size_t wpos = 0;
        while (!slice.IsEnd()) {
            // 首先向左移动缓冲区m个采样，复制m个采样到末尾，然后移动系数相加
            auto block = slice.GetSome(kBatchSize);
            for (size_t i = 0; i < latch_.size() - block.size(); ++i) {
                latch_[i] = latch_[i + block.size()];
            }
            std::copy(block.begin(), block.end(), latch_.end() - block.size());

            for (size_t i = 0; i < block.size(); ++i) {
                float sum = 0.0f;
                for (size_t j = 0; j < coeff_.size(); ++j) {
                    sum += coeff_[j] * latch_[i + j];
                }
                x[wpos++] = sum;
            }
        }
    }

    void Process(std::span<float> in, std::span<float> out) noexcept {
        segement::Slice1D slice{in};
        size_t wpos = 0;
        while (!slice.IsEnd()) {
            // 首先向左移动缓冲区m个采样，复制m个采样到末尾，然后移动系数相加
            auto block = slice.GetSome(kBatchSize);
            for (size_t i = 0; i < latch_.size() - block.size(); ++i) {
                latch_[i] = latch_[i + block.size()];
            }
            std::copy(block.begin(), block.end(), latch_.end() - block.size());

            for (size_t i = 0; i < block.size(); ++i) {
                float sum = 0.0f;
                for (size_t j = 0; j < coeff_.size(); ++j) {
                    sum += coeff_[j] * latch_[i + j];
                }
                out[wpos++] = sum;
            }
        }
    }
private:
    std::vector<float> coeff_;
    std::vector<float> latch_;
};

class FIRTranspose {
public:
    void Reset() noexcept {
        std::fill(latch_.begin(), latch_.end(), 0.0f);
    }

    /**
     * @brief 滤波器的参数是h(0)...h(n-1)排列，且coeff的大小需要手动分配
     * @tparam Func void(std::vector<float>& coeff)
     */
    template <class Func>
    void SetCoeff(Func&& func) {
        func(coeff_);
        if (latch_.size() < coeff_.size() - 1) {
            latch_.resize(coeff_.size() - 1);
        }
        Reset();
    }

    float Tick(float x) noexcept {
        float const y = coeff_.front() * x + latch_.front();
        for (size_t i = 0; i < latch_.size() - 1; ++i) {
            latch_[i] = latch_[i + 1] + coeff_[i + 1] * x;
        }
        latch_.back() = coeff_.back() * x;
        return y;
    }

    void Process(std::span<float> x) noexcept {
        for (auto& s : x) {
            s = Tick(s);
        }
    }

    std::span<const float> GetCoeff() const noexcept {
        return coeff_;
    }
private:
    std::vector<float> coeff_;
    std::vector<float> latch_;
};
}