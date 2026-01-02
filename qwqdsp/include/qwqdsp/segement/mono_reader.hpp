#pragma once
#include <algorithm>
#include <cstddef>
#include <span>

namespace qwqdsp_segement {
class MonoReader {
public:
    MonoReader(std::span<float> source) noexcept 
        : source_(source)
        , rpos_(0)
    {}

    std::span<float> GetSome(size_t size) noexcept {
        size_t can_read = std::min(source_.size() - rpos_, size);
        std::span ret {source_.data() + rpos_, can_read};
        return ret;
    }

    void Read(size_t size, std::span<float> buffer) noexcept {
        size_t can_read = std::min(source_.size() - rpos_, size);
        std::copy_n(source_.data() + rpos_, can_read, buffer.begin());
        std::fill_n(buffer.begin() + can_read, size - can_read, 0.0f);
    }

    void Step(size_t hop) noexcept {
        rpos_ += hop;
    }

    bool IsEnd() const noexcept {
        return rpos_ >= source_.size();
    }

    size_t GetReadpos() const noexcept {
        return rpos_;
    }

    void ReadAbsolute(size_t offset, size_t count, float* buffer) const noexcept {
        if (offset >= source_.size()) {
            std::fill_n(buffer, count, 0);
        }
        else {
            size_t cando = std::min(count, source_.size() - offset);
            std::copy_n(source_.data() + offset, cando, buffer);
            std::fill_n(buffer + cando, count - cando, 0);
        }
    }
private:
    std::span<float> source_;
    size_t rpos_{};
};
}
