/*
 * based on https://github.com/accabog/MedianFilter
 * 
 * MIT License
 * 
 * Copyright (c) 2018 Alexandru Bogdan
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
*/

#pragma once
#include <vector>
#include <cassert>
#include <array>

namespace qwqdsp::filter {
template<class T> requires std::is_trivial_v<T>
class MedianDynamic {
public:
    void Init(size_t window_size) {
        assert(window_size > 2 && window_size % 2 == 1);

        buffer_.resize(window_size);
        Reset();
    }

    void Reset() noexcept {
        size_t const window_size = buffer_.size();
        for(size_t i = 0; i < window_size; i++) {
            buffer_[i].value = T{};
            buffer_[i].next_age = &buffer_[(i + 1) % window_size];
            buffer_[i].next_value = &buffer_[(i + 1) % window_size];
            buffer_[(i + 1) % window_size].prev_value = &buffer_[i];
        }

        age_head_ = buffer_.data();
        value_head_ = buffer_.data();
        median_head_ = &buffer_[window_size / 2];
        first_init_ = true;
    }

    T Tick(T x) noexcept {
        [[unlikely]]
        if (first_init_) {
            first_init_ = false;
            for (auto& node : buffer_) {
                node.value = x;
            }
            return x;
        }

        if(age_head_ == value_head_) {
            age_head_ = value_head_->next_value;
        }

        if((age_head_ == median_head_) || (age_head_->value > median_head_->value)) {
            median_head_ = median_head_->prev_value;
        }

        auto* new_node = age_head_;
        new_node->value = x;

        age_head_->next_value->prev_value = age_head_->prev_value;
        age_head_->prev_value->next_value = age_head_->next_value;
        age_head_ = age_head_->next_age;

        auto* it = value_head_;
        size_t i = 0;
        for(;i < buffer_.size() - 1; i++) {
            if(x < it->value)
            {
                if(i == 0)
                {
                    value_head_ = new_node;
                }
                break;
            }
            it = it->next_value;
        }

        it->prev_value->next_value = new_node;
        new_node->prev_value = it->prev_value;
        it->prev_value = new_node;
        new_node->next_value = it;

        if(i >= (buffer_.size() / 2)) {
            median_head_ = median_head_->next_value;
        }

        return median_head_->value;
    }


    /**
     * @tparam Func std::partial_ordering compare(T const& a, T const & b)
     */
    template<class Func> requires requires (T const& a, T const& b, Func comparator) {
        {comparator(a, b)} -> std::same_as<std::partial_ordering>;
    }
    T Tick(T x, Func&& compare) noexcept(noexcept(compare(std::declval<T>(), std::declval<T>()))) {
        [[unlikely]]
        if (first_init_) {
            first_init_ = false;
            for (auto& node : buffer_) {
                node.value = x;
            }
            return x;
        }

        if(age_head_ == value_head_) {
            age_head_ = value_head_->next_value;
        }

        if((age_head_ == median_head_) || (compare(age_head_->value, median_head_->value) == std::partial_ordering::greater)) {
            median_head_ = median_head_->prev_value;
        }

        auto* new_node = age_head_;
        new_node->value = x;

        age_head_->next_value->prev_value = age_head_->prev_value;
        age_head_->prev_value->next_value = age_head_->next_value;
        age_head_ = age_head_->next_age;

        auto* it = value_head_;
        size_t i = 0;
        for(;i < buffer_.size() - 1; i++) {
            if(compare(x, it->value) == std::partial_ordering::less)
            {
                if(i == 0)
                {
                    value_head_ = new_node;
                }
                break;
            }
            it = it->next_value;
        }

        it->prev_value->next_value = new_node;
        new_node->prev_value = it->prev_value;
        it->prev_value = new_node;
        new_node->next_value = it;

        if(i >= (buffer_.size() / 2)) {
            median_head_ = median_head_->next_value;
        }

        return median_head_->value;
    }
private:
    struct MedianNode {
        T value;
        struct MedianNode *next_age;
        struct MedianNode *next_value;
        struct MedianNode *prev_value;
    };

    std::vector<MedianNode> buffer_;
    MedianNode *age_head_{};
    MedianNode *value_head_{};
    MedianNode *median_head_{};
    bool first_init_{};
};

template<class T, size_t kWindowSize>
class Median {
public:
    static_assert(kWindowSize > 2 && kWindowSize % 2 == 1);
    static_assert(std::is_trivial_v<T>);

    Median() {
        Reset();
    }

    void Reset() noexcept {
        size_t const window_size = buffer_.size();
        for(size_t i = 0; i < window_size; i++) {
            buffer_[i].value = T{};
            buffer_[i].next_age = &buffer_[(i + 1) % window_size];
            buffer_[i].next_value = &buffer_[(i + 1) % window_size];
            buffer_[(i + 1) % window_size].prev_value = &buffer_[i];
        }

        age_head_ = buffer_.data();
        value_head_ = buffer_.data();
        median_head_ = &buffer_[window_size / 2];
        first_init_ = true;
    }

    /**
     * @tparam Func std::partial_ordering compare(T const& a, T const & b)
     */
    template<class Func> requires requires (T const& a, T const& b, Func comparator) {
        {comparator(a, b)} -> std::same_as<std::partial_ordering>;
    }
    T Tick(T x, Func&& compare) noexcept(noexcept(compare(std::declval<T>(), std::declval<T>()))) {
        [[unlikely]]
        if (first_init_) {
            first_init_ = false;
            for (auto& node : buffer_) {
                node.value = x;
            }
            return x;
        }

        if(age_head_ == value_head_) {
            age_head_ = value_head_->next_value;
        }

        if((age_head_ == median_head_) || (compare(age_head_->value, median_head_->value) == std::partial_ordering::greater)) {
            median_head_ = median_head_->prev_value;
        }

        auto* new_node = age_head_;
        new_node->value = x;

        age_head_->next_value->prev_value = age_head_->prev_value;
        age_head_->prev_value->next_value = age_head_->next_value;
        age_head_ = age_head_->next_age;

        auto* it = value_head_;
        size_t i = 0;
        for(;i < buffer_.size() - 1; i++) {
            if(compare(x, it->value) == std::partial_ordering::less)
            {
                if(i == 0)
                {
                    value_head_ = new_node;
                }
                break;
            }
            it = it->next_value;
        }

        it->prev_value->next_value = new_node;
        new_node->prev_value = it->prev_value;
        it->prev_value = new_node;
        new_node->next_value = it;

        if(i >= (buffer_.size() / 2)) {
            median_head_ = median_head_->next_value;
        }

        return median_head_->value;
    }

    T Tick(T x) noexcept {
        [[unlikely]]
        if (first_init_) {
            first_init_ = false;
            for (auto& node : buffer_) {
                node.value = x;
            }
            return x;
        }

        if(age_head_ == value_head_) {
            age_head_ = value_head_->next_value;
        }

        if((age_head_ == median_head_) || (age_head_->value > median_head_->value)) {
            median_head_ = median_head_->prev_value;
        }

        auto* new_node = age_head_;
        new_node->value = x;

        age_head_->next_value->prev_value = age_head_->prev_value;
        age_head_->prev_value->next_value = age_head_->next_value;
        age_head_ = age_head_->next_age;

        auto* it = value_head_;
        size_t i = 0;
        for(;i < buffer_.size() - 1; i++) {
            if(x < it->value)
            {
                if(i == 0)
                {
                    value_head_ = new_node;
                }
                break;
            }
            it = it->next_value;
        }

        it->prev_value->next_value = new_node;
        new_node->prev_value = it->prev_value;
        it->prev_value = new_node;
        new_node->next_value = it;

        if(i >= (buffer_.size() / 2)) {
            median_head_ = median_head_->next_value;
        }

        return median_head_->value;
    }
private:
    struct MedianNode {
        T value;
        struct MedianNode *next_age;
        struct MedianNode *next_value;
        struct MedianNode *prev_value;
    };

    std::array<MedianNode, kWindowSize> buffer_;
    MedianNode *age_head_{};
    MedianNode *value_head_{};
    MedianNode *median_head_{};
    bool first_init_{};
};
}