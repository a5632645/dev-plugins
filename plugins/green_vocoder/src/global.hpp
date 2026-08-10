#pragma once
#include <array>
#include <cstddef>

// ----------------------------------------
// 可配置编译期常量
// ----------------------------------------
namespace global {

// engine 每块处理采样数
static constexpr size_t kBlockSize = 256;

// stft / block lpc 共用的 FFT 尺寸选择（choice index → 实际长度）
static constexpr std::array<int, 5> kStftSizes{256, 512, 1024, 2048, 4096};

// leaky burg lpc
static constexpr int kNumPoles = 80;
static constexpr int kMaxDownsample = 8;

// block burg lpc
static constexpr size_t kMaxPoles = 80;

// 防止滤波器系数病态所需的微小噪声
static constexpr float kNoiseGain = 1e-5f;

// channel vocoder
static constexpr int kMaxOrder = 100;
static constexpr int kMinOrder = 4;

// stft vocoder
static constexpr int kExtraGainSize = 1;
static constexpr int kMaxNumMfcc = 80;
static constexpr int kMinNumMfcc = 8;

// pitch osc
static constexpr int kPitchBlockSize = 1024;
static constexpr int kPitchHopSize = 1024;

// ui
static constexpr int kUiWidth = 750;
static constexpr int kUiHeight = 350;

} // namespace global
