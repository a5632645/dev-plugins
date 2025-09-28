#pragma once
#include <cstdint>

struct I16Point {
    int16_t x{};
    int16_t y{};
};

struct Rectange {
    int16_t x{};
    int16_t y{};
    int16_t w{};
    int16_t h{};

    constexpr Rectange() = default;
    constexpr Rectange(int16_t x, int16_t y, int16_t w, int16_t h) : x(x), y(y), w(w), h(h) {}

    [[nodiscard]]
    constexpr Rectange RemoveFromTop(int16_t amount) {
        Rectange ret = *this;
        ret.h = amount;
        y += amount;
        h -= amount;
        return ret;
    }
    constexpr void RemovedFromTop(int16_t amount) {
        y += amount;
        h -= amount;
    }

    [[nodiscard]]
    constexpr Rectange RemoveFromBottom(int16_t amount) {
        Rectange ret = *this;
        ret.y = y + h - amount; 
        ret.h = h - amount;
        h -= amount;
        return ret;
    }
    constexpr void RemovedFromBottom(int16_t amount) {
        h -= amount;
    }

    [[nodiscard]]
    constexpr Rectange RemoveFromLeft(int16_t amount) {
        Rectange ret = *this;
        ret.w = amount;
        x += amount;
        w -= amount;
        return ret;
    }
    constexpr void RemovedFromLeft(int16_t amount) {
        x += amount;
        w -= amount;
    }

    [[nodiscard]]
    constexpr Rectange RemoveFromRight(int16_t amount) {
        Rectange ret = *this;
        ret.x = x + w - amount;
        ret.w = w - amount;
        w -= amount;
        return ret;
    }
    constexpr void RemovedFromRight(int16_t amount) {
        w -= amount;
    }

    [[nodiscard]]
    constexpr Rectange WithWidth(int16_t width) {
        Rectange ret = *this;
        ret.w = width;
        return ret;
    }

    [[nodiscard]]
    constexpr Rectange WithHeight(int16_t height) {
        Rectange ret = *this;
        ret.h = height;
        return ret;
    }

    [[nodiscard]]
    constexpr Rectange ReduceRatio(float ratioX, float ratioY) {
        int16_t removeX = static_cast<int16_t>(ratioX / 2 * w);
        int16_t removeY = static_cast<int16_t>(ratioY / 2 * h);
        Rectange ret = *this;
        ret.x += removeX;
        ret.y += removeY;
        ret.w -= removeX * 2;
        ret.h -= removeY * 2;
        return ret;
    }

    [[nodiscard]]
    constexpr Rectange Reduce(int16_t dx, int16_t dy) {
        Rectange ret = *this;
        ret.x += dx;
        ret.y += dy;
        ret.w -= dx * 2;
        ret.h -= dy * 2;
        return ret;
    }

    constexpr void Reduced(int16_t dx, int16_t dy) {
        x += dx;
        y += dy;
        w -= dx * 2;
        h -= dy * 2;
    }

    [[nodiscard]]
    constexpr Rectange Expand(int16_t dx, int16_t dy) {
        Rectange ret = *this;
        ret.x -= dx;
        ret.y -= dy;
        ret.w += dx * 2;
        ret.h += dy * 2;
        return ret;
    }

    constexpr void Translated(int16_t dx, int16_t dy) {
        x += dx;
        y += dy;
    }

    [[nodiscard]]
    constexpr Rectange Translate(int16_t dx, int16_t dy) const {
        return Rectange(x + dx, y + dy, w, h); 
    }

    constexpr I16Point GetCenter() const {
        I16Point p{};
        p.x = x + w / 2;
        p.y = y + h / 2;
        return p;
    }
};
