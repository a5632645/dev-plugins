## 命名

| 类别 | 风格 | 示例 |
|------|------|------|
| 命名空间 | `lower_snake_case` | `qwqdsp_cephes`, `qwqdsp_oscillator` |
| 结构体/类 | `PascalCase` | `struct Bessel`, `class EllipticSineOsc` |
| 函数/方法 | `camelCase` | `GetWorkDir()`, `OutputFile()`, `i0()` |
| 普通变量 | `snake_case` | `bf_i0`, `total_errs` |
| 成员变量 | `snake_case_` | `sample_rate_`, `phase_` |
| 编译期常量 | `k` 前缀 + PascalCase | `kNum`, `kA`, `kPIO2`, `kTableSize` |
| 宏定义 | `UPPER_SNAKE_CASE` | `QWQDSP_WORK_DIR`, `QWQDSP_HAVE_IPP` |

## 注释

- **仅在较复杂的函数或表意不明的变量才写注释**
- Doxygen 使用 JavaDoc 风格 `/** */`
- 行间注释使用`//`
- 特别的注释段使用如下示例
```
// ------------------------------------------------------------
// Foo
// ------------------------------------------------------------

// ----- Bar -----
```

## C++ 标准

- C++20
- 不会抛出异常的函数标记 `noexcept`，禁止在**使用动态内存分配**/**调用了无noexcept标记**的函数上标记
- 系数数组使用 `static constexpr`
- 头文件使用 `#pragma once`
