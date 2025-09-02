#pragma once
#include <algorithm>
#include <cstddef>
#include <span>
#include <vector>

namespace qwqdsp::segement {
/**
 * @brief 切分单通道数据
 */
template<class T>
class Slice1D {
public:
    Slice1D(std::span<T> source) noexcept 
        : source_(source)
        , rpos_(0)
    {}

    std::span<T> GetSome(size_t size, size_t hop) noexcept {
        size_t can_read = std::min(source_.size() - rpos_, size);
        std::span ret {source_.data() + rpos_, can_read};
        rpos_ += hop;
        return ret;
    }

    std::span<T> GetSome(size_t size) noexcept {
        return GetSome(size, size);
    }

    void Read(size_t size, size_t hop, std::span<T> buffer) noexcept {
        size_t can_read = std::min(source_.size() - rpos_, size);
        std::copy_n(source_.data() + rpos_, can_read, buffer.begin());
        std::fill_n(buffer.begin() + can_read, size - can_read, 0.0f);
        rpos_ += hop;
    }

    void Read(size_t size, std::span<T> buffer) noexcept {
        Read(size, size, buffer);;
    }

    bool IsEnd() const noexcept {
        return rpos_ >= source_.size();
    }
private:
    std::span<T> source_;
    size_t rpos_{};
};

/**
 * @brief 切分多通道数据
 */
template<class VecVec>
    requires requires (VecVec v) {
        v.size();
        v[0];
        v[0].size();
        v[0].data();
    }
class Slice2D {
public:
    Slice2D(VecVec& source) 
        : source_(source)
    {
        rpos_.resize(source_.size());
    }

    std::span<float> GetSome(size_t channel, size_t size, size_t hop) noexcept {
        size_t can_read = std::min(source_[channel].size() - rpos_[channel], size);
        std::span ret {source_[channel].data() + rpos_[channel], can_read};
        rpos_[channel] += hop;
        return ret;
    }

    void Read(size_t channel, size_t size, size_t hop, std::span<float> buffer) noexcept {
        size_t can_read = std::min(source_[channel].size() - rpos_[channel], size);
        std::copy_n(source_[channel].data() + rpos_[channel], can_read, buffer.begin());
        std::fill_n(buffer.begin() + can_read, size - can_read, 0.0f);
        rpos_[channel] += hop;
    }


    bool IsEnd(size_t channel) const noexcept {
        return rpos_[channel] >= source_[channel].size();
    }
private:
    VecVec& source_;
    std::vector<size_t> rpos_;
};
}