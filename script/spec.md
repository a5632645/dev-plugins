# 发布流程使用说明

## 列出可发布的插件

```bash
python script/list_plugin.py
```

## 发布某个插件

```bash
python script/release_plugin.py <PluginName@version>
# 示例:
python script/release_plugin.py VitalReverb@0.2.0
python script/release_plugin.py SteepFlanger@0.2.0
```

这个脚本会:
1. 更新 `version.json` 中的版本号
2. 更新 `cmake` 中的版本号
3. 本地 Release 编译该插件
3. 创建 git commit + git tag（格式 `PluginName@version`）
4. push 到远程仓库

## CI 自动发布

push tag 后 GitHub Action 会自动:
1. 在 Windows / macOS / Linux 上编译该插件
2. 上传产物并创建 GitHub Release

注意: git tag 格式必须为 `PluginName@version`（如 `VitalReverb@0.2.0`），`PluginName` 需与 `list_plugin.py` 列出的名称一致。
