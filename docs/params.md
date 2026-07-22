# 参数系统规范 (Parameter System Convention)

本文档定义 dev-plugins 项目中所有插件的参数定义、存储和跨线程同步的统一规范，供开发者及 AI 代理查阅和遵循。

---

## 核心原则

1. **参数定义** — 使用 `pluginshared::wrap_parameters.hpp` 中的包装类（`FloatParam` / `IntParam` / `BoolParam` / `ChoiceParam`）声明所有参数实体，集中在 `Params` 类中
2. **参数监听** — `Params` 自继承 `juce::AudioProcessorParameter::Listener`，内部持有 `std::atomic<bool> changed_`
3. **参数消费** — 音频线程通过 `IsParamChanged()` + `ToXxxParam()` 一次性拉取所有参数

---

## 架构总览

```
┌─────────────────────────────────────────────────────┐
│                  参数源头 (DAW/UI)                    │
│   JUCE AudioProcessorValueTreeState (APVTS)         │
└───────────────┬─────────────────────────────────────┘
                │ 参数值变更
                ▼
┌─────────────────────────────────────────────────────┐
│  Params (juce::AudioProcessorParameter::Listener)    │
│                                                     │
│  ┌─────────────────────────────────────────────┐    │
│  │  wrap_param 实体                              │    │
│  │  (FloatParam, IntParam, BoolParam, ...)      │    │
│  │  .Get() 读取最新值                            │    │
│  └─────────────────────────────────────────────┘    │
│                                                     │
│  parameterValueChanged() ──store(release)──►changed_│
└───────────────┬─────────────────────────────────────┘
                │ IsParamChanged() (acquire)
                ▼
┌─────────────────────────────────────────────────────┐
│  音频线程 (processBlock)                              │
│                                                     │
│  if (changed):                                       │
│    dsp_.Update(ToXxxParam())         ← 原地构造     │
└─────────────────────────────────────────────────────┘
```

---

## 包装类说明 (`wrap_parameters.hpp`)

| 包装类 | JUCE 底层类型 | `Get()` 返回 | 构造函数签名 |
|--------|---------------|-------------|-------------|
| `FloatParam` | `AudioParameterFloat` | `float` | `(name, NormalisableRange<float>, default)` |
| `IntParam` | `AudioParameterInt` | `int` | `(name, min, max, default)` |
| `BoolParam` | `AudioParameterBool` | `bool` | `(name, default)` |
| `ChoiceParam` | `AudioParameterChoice` | `int` (index) | `(name, StringArray, default_name)` |

所有包装类支持：
- `Build()` — 生成 `unique_ptr<JUCE 参数>` 并保存原始指针
- `Get()` — 读取当前值
- `operator+=` — 加入 APVTS layout

---

## `Params` 类规范

### 必需接口

```cpp
class Params : public juce::AudioProcessorParameter::Listener {
public:
    // 1. 将所有参数加入 APVTS layout
    void BuildLayout(juce::AudioProcessorValueTreeState::ParameterLayout& layout);

    // 2. 将所有参数值打包为 DSP 数据结构
    [[nodiscard]] XxxParam ToXxxParam();

    // 3. 注册/注销 Listener
    void BeginListening();
    void EndListening();

    // 4. 消费变更标记
    bool IsParamChanged() noexcept;   // exchange(false, acquire)
    void MarkChanged() noexcept;       // store(true, release)

    std::atomic<bool> changed_{false};

private:
    // 5. Listener 回调
    void parameterValueChanged(int, float) override;   // store(true, release)
    void parameterGestureChanged(int, bool) override;
};
```

### 最小示例

```cpp
// params.hpp
#pragma once
#include <pluginshared/wrap_parameters.hpp>
#include "dsp/idsp.hpp"

class Params : public juce::AudioProcessorParameter::Listener {
public:
    // --- 参数实体 ---
    pluginshared::FloatParam mix{"Mix", {0.0f, 1.0f, 0.01f}, 0.5f};
    pluginshared::BoolParam bypass{"bypass", false};
    pluginshared::ChoiceParam mode{
        "mode",
        juce::StringArray{"A", "B", "C"},
        "A"
    };

    // --- 构建 layout ---
    void BuildLayout(juce::AudioProcessorValueTreeState::ParameterLayout& layout) {
        layout += mix;
        layout += bypass;
        layout += mode;
    }

    // --- 打包为 DSP 数据结构 ---
    [[nodiscard]] DspParam ToDspParam() {
        DspParam p;
        p.mix = mix.Get();
        p.mode = static_cast<DspMode>(mode.Get());
        return p;
    }

    // --- Listener 注册 ---
    void BeginListening() {
        mix.ptr_->addListener(this);
        bypass.ptr_->addListener(this);
        mode.ptr_->addListener(this);
    }
    void EndListening() {
        mix.ptr_->removeListener(this);
        bypass.ptr_->removeListener(this);
        mode.ptr_->removeListener(this);
    }

    // --- 变更标记 ---
    bool IsParamChanged() noexcept { return changed_.exchange(false, std::memory_order_acquire); }
    void MarkChanged() noexcept { changed_.store(true, std::memory_order_release); }
    std::atomic<bool> changed_{false};

private:
    void parameterValueChanged(int parameterIndex, float newValue) override {
        juce::ignoreUnused(parameterIndex, newValue);
        changed_.store(true, std::memory_order_release);
    }
    void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override {
        juce::ignoreUnused(parameterIndex, gestureIsStarting);
    }
};
```

---

## `PluginProcessor` 集成

### 头文件

```cpp
// PluginProcessor.h
#include "params.hpp"

class MyAudioProcessor final : public juce::AudioProcessor {
public:
    // ...
    Params params_;
    std::unique_ptr<juce::AudioProcessorValueTreeState> value_tree_;
    std::unique_ptr<pluginshared::PresetManager> preset_manager_;
    // 不再持有 atomic<bool> —— 已移至 Params 内部
};
```

### 构造函数

```cpp
MyAudioProcessor::MyAudioProcessor()
    : AudioProcessor(...) {
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    params_.BuildLayout(layout);

    value_tree_ = std::make_unique<juce::AudioProcessorValueTreeState>(
        *this, nullptr, "PARAMETERS", std::move(layout));
    params_.BeginListening();

    preset_manager_ = std::make_unique<pluginshared::PresetManager>(*value_tree_, *this);
}
```

### 析构函数

```cpp
MyAudioProcessor::~MyAudioProcessor() {
    params_.EndListening();       // 必须在 value_tree_ 销毁前调用
    preset_manager_ = nullptr;
    value_tree_ = nullptr;
}
```

### 音频线程

```cpp
void MyAudioProcessor::processBlock(...) {
    if (params_.IsParamChanged())
        dsp_.Update(params_.ToDspParam());
    // ... process audio ...
}
```

### prepareToPlay

确保初始参数同步：

```cpp
void MyAudioProcessor::prepareToPlay(...) {
    dsp_.Init(fs);
    dsp_.Reset();
    params_.MarkChanged();     // 触发首次 Update
}
```

---

## 内存序规范

使用 **release-acquire** 而非默认 `seq_cst`：

| 操作 | 代码 | order |
|------|------|-------|
| 标记变更 | `changed_.store(true, ...)` | `memory_order_release` |
| 消费变更 | `changed_.exchange(false, ...)` | `memory_order_acquire` |

**保证**：UI 线程对参数值（`ptr_->get()` 背后的内存）的所有写入，在音频线程读到 `changed_ == true` 后全部可见。

---

## preset 加载说明

`PresetManager` 通过 APVTS 的 `replaceState` 加载 preset，这会逐个触发所有参数的 `parameterValueChanged` 回调。因此不需要额外处理 —— 每个 Listener 回调都会标记 `changed_`，音频线程在下一次 `processBlock` 中自动拉取。

但注意 preset 加载可能在音频线程外发生，且可能密集触发多个回调。`changed_` 只需一个 bit，多次 `store(release)` 是安全的。

---

## 多模块分解

在插件较为简单时，一个 `Params` 类足以容纳所有参数。但当插件涉及多个独立 DSP 模块（如振荡器 + 滤波器 + 混响 + LFO 等），推荐将 `Params` 拆分为多个模块级的 Param 类，每个模块拥有独立的 `atomic<bool>` 和 Listener，在音频线程上独立消费。

### 示例：合成器多模块架构

```cpp
// params.hpp
class OscParam : public juce::AudioProcessorParameter::Listener { /* ... */ };
class FilterParam : public juce::AudioProcessorParameter::Listener { /* ... */ };
class ReverbParam : public juce::AudioProcessorParameter::Listener { /* ... */ };

class Params {
public:
    OscParam osc;
    FilterParam filter;
    ReverbParam reverb;

    void BuildLayout(juce::AudioProcessorValueTreeState::ParameterLayout& layout) {
        osc.BuildLayout(layout);
        filter.BuildLayout(layout);
        reverb.BuildLayout(layout);
    }

    void BeginListening() {
        osc.BeginListening();
        filter.BeginListening();
        reverb.BeginListening();
    }

    void EndListening() {
        osc.EndListening();
        filter.EndListening();
        reverb.EndListening();
    }
};
```

### 音频线程独立消费

```cpp
void processBlock(...) {
    if (osc.IsParamChanged())     dsp_.UpdateOsc(osc.ToOscParam());
    if (filter.IsParamChanged())  dsp_.UpdateFilter(filter.ToFilterParam());
    if (reverb.IsParamChanged())  dsp_.UpdateReverb(reverb.ToReverbParam());
    // ... process audio ...
}
```

### 原则

- 每个模块 Param 类复用同一套 `wrap_param` 包装类和 Listener 模式
- 每个模块持有自己的 `std::atomic<bool> changed_`，互不干扰
- 音频线程按模块独立检查、独立更新，避免无关参数变更触发不必要的 DSP 更新
- `BuildLayout()` 仍由顶层 `Params` 统一编排，确保所有参数在同一个 APVTS 中

---

## 添加新参数步骤（AI 代理模板）

1. **`params.hpp`** — 在 `Params` 类中添加包装类实体
2. **`params.hpp`** — 在 `BuildLayout()` 中添加 `layout += xxx;`
3. **`params.hpp`** — 在 `ToDspParam()` 中添加映射
4. **`params.hpp`** — 在 `BeginListening()` / `EndListening()` 中添加 add/remove
5. **`idsp.hpp`**（或对应 DSP 头文件）— 若尚无对应字段则添加
