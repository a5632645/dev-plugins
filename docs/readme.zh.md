# dev-plugins

ManasWorld 开发的音频插件

网站：https://manaswolrd.github.io/en/index.html

---

## 插件列表

| 名称 | 简介 |
|------|------|
| `DeepPhaser` | barberpole FIR 移相器 |
| `DispersiveDelay` | 全通滤波器链实现的色散延迟 |
| `GreenVocoder` | 多算法声码器 |
| `Resonator` | 非谐共振器，带有散射矩阵 |
| `STTR` | 短时时间反向 —— [`short-time-time-reversal`](https://ccrma.stanford.edu/~hskim08/sttr/dev.html) |
| `SteepFlanger` | barberpole FIR/IIR 镶边 |
| `VitalChorus` | Vital 合成器合唱效果移植 |
| `VitalReverb` | Vital 合成器混响效果移植 |
| `WarpCore` | 多频段频谱反转 |

---

## 开发者 / AI 代理

### 发布新版本

使用 `release_plugin.py` 脚本：

```bash
python script/release_plugin.py PluginName@version
```

示例：`python script/release_plugin.py WarpCore@0.1.0`

该命令会更新版本号、编译 Release、创建 git tag 并推送，触发发布工作流。

### CI 测试触发

#### 提交信息格式

符合规范的作用域提交可触发 CI 测试：

```
type(PluginName): description
```

- **type** — 小写，例如 `fix`、`feat`、`refactor`、`chore`、`docs`
- **PluginName** — 插件名，来自 `python script/list_plugin.py`，例如 `WarpCore`、`DeepPhaser`
- **作用域必须是有效的插件名** — 未知作用域会被静默跳过

#### 示例

| 提交信息 | 行为 |
|----------|------|
| `fix(WarpCore): fix LFO sync` | ✅ 触发 WarpCore 测试 |
| `feat(SteepFlanger): add feedback` | ✅ 触发 SteepFlanger 测试 |
| `docs(readme): update` | ⏭️ `readme` 不是插件，跳过 |
| `chore(ci): fix workflow` | ⏭️ `ci` 不是插件，跳过 |
| `Fix(WarpCore): ...` | ⏭️ type 必须小写 |
| `feat(WarpCore)!: breaking change` | ⏭️ `!` 不被支持 |
| `update stuff` | ⏭️ 无作用域格式，跳过 |
| `workflow_dispatch` | ✅ 通过手动输入触发 |

#### 注意

- type **必须小写**（`fix` ✅ / `Fix` ❌）
- 插件名必须匹配 `python script/list_plugin.py` 中的目标（`WarpCore` ✅ / `warpcore` ❌）
- `)` 和 `:` 之间不能有空格（即 `type(PluginName):`），但 `:` 后可以有空格
- 手动触发时直接输入插件名，无需提交格式
