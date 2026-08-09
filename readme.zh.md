# dev-plugins

ManasWorld 开发的音频插件

网站：https://manaswolrd.github.io/en/index.html

[English](readme.md)

## AI / LLM

本仓库包含 AI/LLM 辅助编写的代码和文档。

如果你不希望使用涉及 AI 的项目，那么这个仓库可能不适合你。

---

## 插件列表

点击对应插件名字的链接查看更详细的介绍  

| 名称 | 简介 |
|------|------|
| `DeepPhaser` | barberpole FIR 移相器 |
| `DispersiveDelay` | 大规模全通滤波器实现的色散延迟 |
| [`GreenVocoder`](docs/greenvocoder.zh.md) | 多算法声码器 |
| `Resonator` | 带散射矩阵的非谐共振器 |
| [`STTR`](docs/sttr.zh.md) | 即 [`short-time-time-reversal`](https://ccrma.stanford.edu/~hskim08/sttr/dev.html) |
| [`SteepFlanger`](docs/steepflanger.zh.md) | barberpole FIR/IIR 镶边 |
| `VitalChorus` | Vital 合成器合唱效果的移植 |
| [`VitalReverb`](docs/vitalreverb.md) | Vital 合成器混响效果的移植 |
| [`WarpCore`](docs/warpcore.md) | 多频段频谱反转 |

---

## 安装插件

从 Release 下载对应操作系统的压缩包。解压后通常会看到类似这样的结构：

```text
pluginName-win-vX.Y.Z.zip
  VST3/
    PluginName.vst3/

pluginName-macos-vX.Y.Z.zip
  AU/
    PluginName.component/
  VST3/
    PluginName.vst3/

pluginName-linux-vX.Y.Z.zip
  LV2/
    PluginName.lv2/
  VST3/
    PluginName.vst3/
```

安装时直接复制其中一个文件夹：

- `PluginName.vst3`
- `PluginName.component`
- `PluginName.lv2`

常见的安装位置：

- Windows VST3：`C:\Program Files\Common Files\VST3\`
- macOS VST3：`/Library/Audio/Plug-Ins/VST3/` 或 `~/Library/Audio/Plug-Ins/VST3/`
- macOS AU：`/Library/Audio/Plug-Ins/Components/` 或 `~/Library/Audio/Plug-Ins/Components/`
- Linux LV2：`~/.lv2/` 或 `/usr/lib/lv2/`
- Linux VST3：`~/.vst3/` 或 `/usr/lib/vst3/`

例如在 Windows 上，不要复制 `VST3` 文件夹本身，而是把它里面的整个 `PluginName.vst3` 文件夹复制到 `C:\Program Files\Common Files\VST3\`。

另外，macOS 用户可能还需要执行以下命令：

```bash
sudo xattr -dr com.apple.quarantine /path/to/your/plugins/plugin_name.component
sudo xattr -dr com.apple.quarantine /path/to/your/plugins/plugin_name.vst3
sudo xattr -dr com.apple.quarantine /path/to/your/plugins/plugin_name.lv2
```

如果 macOS 阻止了下载的插件，可以对插件 bundle 运行上面的命令来移除隔离属性。

---

## 开发者 / AI 代理

参见 [developer.zh.md](developer.zh.md)

## 致谢

STTR 研究者，https://ccrma.stanford.edu/~hskim08/sttr/  
surge 团队的 sst-basic-blocks，https://github.com/surge-synthesizer
