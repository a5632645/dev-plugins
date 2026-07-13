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
| `GreenVocoder` | multiple algorithm vocoder |
| `Resonator` | inharmonic resonator with scatter matrix |
| `STTR` | aka [`short-time-time-reversal`](https://ccrma.stanford.edu/~hskim08/sttr/dev.html) |
| `SteepFlanger` | barberpole FIR/IIR flanger |
| `VitalChorus` | port of vital synth's chorus |
| `VitalReverb` | port of vital synth's reverb |
| `WarpCore` | multiband spectral inversion |

---

## for developers and agent

### release a new plugin version

Use the `release_plugin.py` script:

```bash
python script/release_plugin.py PluginName@version
```

Example: `python script/release_plugin.py WarpCore@0.1.0`

This updates the version, builds Release, creates a git tag, and pushes — triggering the release workflow.

### CI test trigger

#### Commit message format

A conventional commit with a plugin scope triggers CI tests:

```
type(PluginName): description
```

- **type** — lowercase, e.g. `fix`, `feat`, `refactor`, `chore`, `docs`
- **PluginName** — name from `python script/list_plugin.py`, e.g. `WarpCore`, `DeepPhaser`
- **scope must be a valid plugin name** — unknown scopes are silently skipped

#### Examples

| commit | behavior |
|--------|----------|
| `fix(WarpCore): fix LFO sync` | ✅ triggers WarpCore test |
| `feat(SteepFlanger): add feedback` | ✅ triggers SteepFlanger test |
| `docs(readme): update` | ⏭️ `readme` is not a plugin, skipped |
| `chore(ci): fix workflow` | ⏭️ `ci` is not a plugin, skipped |
| `Fix(WarpCore): ...` | ⏭️ type must be lowercase |
| `feat(WarpCore)!: breaking change` | ⏭️ `!` not supported in regex |
| `update stuff` | ⏭️ no scope format, skipped |
| `workflow_dispatch` | ✅ trigger via manual input |

#### Notes

- type must be **lowercase** (`fix` ✅ / `Fix` ❌)
- plugin name must match a target from `python script/list_plugin.py` (`WarpCore` ✅ / `warpcore` ❌)
- no space between `)` and `:` (i.e. `type(PluginName):`), space after `:` is fine
- manual dispatch accepts the plugin name directly, no commit format needed
