#pragma once
#include <vector>
#include <span>

namespace qwqdsp::misc {
class PeakFinding {
public:
    void Init(size_t data_len) {
        min_peaks_.resize(data_len);
        max_peaks_.resize(data_len);
    }

    void Process(std::span<float const> data, bool max_first, float delta) noexcept {
        min_peaks_.clear();
        max_peaks_.clear();

        size_t max_pos = 0;
        size_t min_pos = 0;
        bool look_for_max = max_first;
        float max = data[0];
        float min = data[0];

        for(size_t i = 1; i < data.size(); ++i) {
            if(data[i] > max) {
                max_pos = i;
                max = data[i];
            }
            if(data[i] < min) {
                min_pos = i;
                min = data[i];
            }

            if(look_for_max && data[i] < max - delta) {
                max_peaks_.push_back(max_pos);
                look_for_max = false;
                i = max_pos - 1;
                min = data[max_pos];
                min_pos = max_pos;
            }
            else if((!look_for_max) && data[i] > min + delta) {
                min_peaks_.push_back(min_pos);
                look_for_max = true;
                i = min_pos - 1;
                max = data[min_pos];
                max_pos = min_pos;
            }
        }
    }

    std::vector<size_t> const& GetMaxIndexs() const noexcept {
        return max_peaks_;
    }

    std::vector<size_t> const& GetMinIndexs() const noexcept {   
        return min_peaks_;
    }
private:
    std::vector<size_t> min_peaks_;
    std::vector<size_t> max_peaks_;
};
}