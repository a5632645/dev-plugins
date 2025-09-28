#pragma once
#include <span>
#include <vector>

namespace qwqdsp::misc {
/**
 * @ref https://zhuanlan.zhihu.com/p/549588865
 */
class AMPDPeakFinding {
public:
    void Init(size_t data_length) {
        results_.reserve(data_length);
        p_data_.resize(data_length);
    }

    template <class T>
    [[nodiscard]]
    std::vector<size_t> const& Process(std::span<T> data) noexcept {
        int min_row_sum = 0;
        size_t max_window_len = 0;
        for (size_t k = 1; k < data.size() / 2 + 1; ++k) {
            int sum = 0;
            for (size_t i = k; i < data.size() - k; ++i) {
                if (data[i] > data[i - k] && data[i] > data[i + k]) {
                    --sum;
                }
            }

            if (sum < min_row_sum) {
                min_row_sum = sum;
                max_window_len = k - 1;
            }
        }

        results_.clear();
        if (max_window_len == 0) {
            for (size_t i = 1; i < data.size() - 1; ++i) {
                if (data[i] > data[i - 1] && data[i] > data[i + 1]) {
                    results_.push_back(i);
                }
            }
        }
        else {
            std::fill(p_data_.begin(), p_data_.end(), 0);
            for (size_t k = 1; k < max_window_len + 1; ++k) {
                for (size_t i = k; i < data.size() - k; ++i) {
                    if (data[i] > data[i - k] && data[i] > data[i + k]) {
                        ++p_data_[i];
                    }
                }
            }
            
            for (size_t i = 0; i < p_data_.size(); ++i) {
                if (p_data_[i] == max_window_len) {
                    results_.push_back(i);
                }
            }
        }

        return results_;
    }
private:
    std::vector<size_t> results_;
    std::vector<size_t> p_data_;
};
}