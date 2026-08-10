#pragma once
#include <cstddef>

namespace global {

static constexpr size_t kMaxCoeffLen = 64;
static constexpr size_t kFFTSize = 512;
static constexpr float kMaxDelayMs = 20.0f;
static constexpr float kModuDelayMs = 10.0f;
// 初始化延迟线/IIR 所需的最大长度（略大于最大延迟 + 调制深度）
static constexpr float kInitMaxMs = kMaxDelayMs + kModuDelayMs + 0.1f;

static constexpr size_t kIirMaxNumFilters = 16;

static constexpr size_t kSIMDMaxCoeffLen = ((global::kMaxCoeffLen + 7) / 8) * 8;
static constexpr float kDelaySmoothMs = 20.0f;

} // namespace global
