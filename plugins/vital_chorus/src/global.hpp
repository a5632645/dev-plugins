#pragma once

namespace global {

static constexpr float kMaxModulationMs = 30;                                         // 最大 LFO 调制深度（ms）
static constexpr float kMaxStaticDelayMs = 20;                                        // 最大静态延迟（ms）
static constexpr float kMaxDelayMs = kMaxStaticDelayMs + kMaxModulationMs * 1.5f + 1; // 延迟线最大容量（ms）
static constexpr int kMaxNumChorus = 16;                                              // 最大声道数（4 的倍数）

} // namespace global
