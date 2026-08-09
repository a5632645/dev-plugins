# dev-plugins
plugins by manaswolrd  

website: https://manaswolrd.github.io/en/index.html  

[中文版 / Chinese](docs/readme.zh.md)

## AI / LLM

This repository contains AI/LLM-assisted code and documentation.

If you prefer not to use projects involving AI, this repository probably isn't for you.

---

## plugins

| name | description |
|----------|------|
| `DeepPhaser` | barberpole FIR phaser |
| `DispersiveDelay` | massive allpass filter create dispersive delay |
| [`GreenVocoder`](docs/greenvocoder.md) | multiple algorithm vocoder |
| `Resonator` | inharmonic resonator with scatter matrix |
| [`STTR`](docs/sttr.md) | aka [`short-time-time-reversal`](https://ccrma.stanford.edu/~hskim08/sttr/dev.html) |
| [`SteepFlanger`](docs/steepflanger.md) | barberpole FIR/IIR flanger |
| `VitalChorus` | port of vital synth's chorus |
| [`VitalReverb`](docs/vitalreverb.md) | port of vital synth's reverb |
| [`WarpCore`](docs/warpcore.md) | multiband spectral inversion |

---

## Install Plugin

Download the compressed package for the corresponding operating system from Release. After extracting it, you will generally see a structure like this:  

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

When installing, copy one of these folders directly:

- `PluginName.vst3`
- `PluginName.component`
- `PluginName.lv2`

Common install locations:

- Windows VST3: `C:\Program Files\Common Files\VST3\`
- macOS VST3: `/Library/Audio/Plug-Ins/VST3/` or `~/Library/Audio/Plug-Ins/VST3/`
- macOS AU: `/Library/Audio/Plug-Ins/Components/` or `~/Library/Audio/Plug-Ins/Components/`
- Linux LV2: `~/.lv2/` or `/usr/lib/lv2/`
- Linux VST3: `~/.vst3/` or `/usr/lib/vst3/`

For example, on Windows, do not copy the `VST3` folder itself. Copy the whole `PluginName.vst3` folder inside it to `C:\Program Files\Common Files\VST3\`.

Additionally, macOS users may need to do the following:

```bash
sudo xattr -dr com.apple.quarantine /path/to/your/plugins/plugin_name.component
sudo xattr -dr com.apple.quarantine /path/to/your/plugins/plugin_name.vst3
sudo xattr -dr com.apple.quarantine /path/to/your/plugins/plugin_name.lv2
```

If macOS blocks a downloaded plugin, you can run the commands above on the plugin bundle to remove the quarantine attribute.

---

## for developers and agents
see [developer.md](docs/developer.md)

## thanks
sttr researchers, https://ccrma.stanford.edu/~hskim08/sttr/  
surge team for the sst-basic-blocks, https://github.com/surge-synthesizer   
