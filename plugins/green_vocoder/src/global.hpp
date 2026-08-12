#pragma once
#include <array>

namespace global {

// engine 每块处理采样数
static constexpr int kBlockSize = 256;

// stft / block lpc 共用的 FFT 尺寸选择（choice index → 实际长度）
static constexpr std::array<int, 5> kStftSizes{256, 512, 1024, 2048, 4096};

// leaky burg lpc
static constexpr int kNumPoles = 80;
static constexpr int kMaxDownsample = 8;

// block burg lpc
static constexpr int kMaxPoles = 80;
// 岭回归常数：阻止反射系数计算（k = -2*up/(down+λ)）除零病态
static constexpr float kRidge = 1e-4f;

// 防止滤波器系数病态所需的微小噪声（leaky burg 使用）
static constexpr float kNoiseGain = 1e-5f;

// channel vocoder
static constexpr int kMaxOrder = 100;
static constexpr int kMinOrder = 4;

// stft vocoder
static constexpr int kExtraGainSize = 2; // gains 尾部 2 个 0 bin（formant 顶部平滑衰减用）
static constexpr int kMaxNumMfcc = 80;
static constexpr int kMinNumMfcc = 8;
// 非 Morph STFT 模式的 mod 增益补偿（乘在 Blend 之前）：默认 blend 下提升输出电平，
// 但 blend（noisy）增大时增益仍按 Blend 曲线饱和逼近 1，行为不变
static constexpr float kStftModMakeup = 10.0f;

// pitch osc
static constexpr int kPitchBlockSize = 1024;
static constexpr int kPitchHopSize = 1024;

// ui
static constexpr int kUiWidth = 750;
static constexpr int kUiHeight = 350;

} // namespace global
