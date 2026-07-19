"""
本地发布脚本。
用法: python release_plugin.py <PluginName@version>
  例如: python release_plugin.py VitalReverb@0.2.0
"""

import json
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
VERSION_JSON = ROOT / "version.json"
BUILD_DIR = ROOT / "build"


def die(msg: str):
    print(f"Error: {msg}", file=sys.stderr)
    sys.exit(1)


def _format_cmd(cmd: list[str]) -> str:
    """格式化命令显示，对含空格的参数加引号以便于阅读。"""
    return " ".join(arg if " " not in arg else f'"{arg}"' for arg in cmd)


def run(cmd: list[str], cwd=None):
    print(f"$ {_format_cmd(cmd)}")
    result = subprocess.run(cmd, cwd=cwd or ROOT)
    if result.returncode != 0:
        die(f"command failed: {_format_cmd(cmd)}")
    return result


def run_git_commit(cmd: list[str], cwd=None):
    """执行 git commit，若返回"nothing to commit"视为成功。"""
    print(f"$ {_format_cmd(cmd)}")
    result = subprocess.run(cmd, cwd=cwd or ROOT, capture_output=True, text=True)
    if result.returncode != 0:
        combined = result.stdout + result.stderr
        if "nothing to commit" in combined or "no changes added to commit" in combined:
            print("(nothing to commit, skipping)")
            return result
        die(f"command failed: {_format_cmd(cmd)}")
    return result


def update_version_json(target: str, version: str, tag: str):
    with open(VERSION_JSON, encoding="utf-8") as f:
        data = json.load(f)

    found = False
    for entry in data:
        if entry["name"] == target:
            entry["version"] = version
            entry["tag"] = tag
            found = True
            break

    if not found:
        data.append({"name": target, "version": version, "tag": tag})

    with open(VERSION_JSON, "w", encoding="utf-8", newline="\n") as f:
        json.dump(data, f, indent=4)
        f.write("\n")


def update_cmake_version(cmake_file: Path, version: str):
    text = cmake_file.read_text(encoding="utf-8")
    updated, count = re.subn(
        r"(^\s*set\s*\(\s*PLUGIN_VERSION\s+)([^)\s]+)(\s*\))",
        rf"\g<1>{version}\g<3>",
        text,
        count=1,
        flags=re.MULTILINE,
    )
    if count != 1:
        die(f"PLUGIN_VERSION not found in {cmake_file.relative_to(ROOT)}")
    cmake_file.write_text(updated, encoding="utf-8", newline="\n")


def main():
    if len(sys.argv) != 2:
        die("Usage: python release_plugin.py <PluginName@version>")

    arg = sys.argv[1]
    parts = arg.split("@")
    if len(parts) != 2:
        die(f"Invalid format '{arg}'. Expected 'PluginName@version'")

    name, version = parts[0], parts[1]

    # 确认 CMake target 存在
    from names import cmake_file_for, resolve
    target = resolve(name)
    if not target:
        die(f"Unknown plugin name: {name}")

    cmake_file = cmake_file_for(target)
    if not cmake_file:
        die(f"CMake file not found for plugin: {target}")

    print(f"--- Release {target} @ {version} ---")

    tag = f"{target}@{version}"

    # 更新版本来源，CI 会基于这次提交构建产物。
    update_version_json(target, version, tag)

    # 本地 Release 构建
    update_cmake_version(cmake_file, version)
    run(["cmake", "--build", str(BUILD_DIR), "--target", f"{target}_All", "--config", "RelWithDebInfo"])

    # git commit & tag
    run(
        [
            "git",
            "add",
            str(VERSION_JSON.relative_to(ROOT)),
            str(cmake_file.relative_to(ROOT)),
        ]
    )
    run_git_commit(["git", "commit", "-m", f"release {target}@{version}"])
    run(["git", "tag", tag])
    run(["git", "push"])
    run(["git", "push", "--tags"])

    print(f"\nDone. Pushed tag {tag}. GitHub Action will build and release.")


if __name__ == "__main__":
    main()
