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


def run(cmd: list[str], cwd=None):
    print(f"$ {' '.join(cmd)}")
    result = subprocess.run(cmd, cwd=cwd or ROOT)
    if result.returncode != 0:
        die(f"command failed: {' '.join(cmd)}")
    return result


def update_version_json(target: str, version: str):
    with open(VERSION_JSON, encoding="utf-8") as f:
        data = json.load(f)

    found = False
    for entry in data:
        if entry["name"] == target:
            entry["version"] = version
            found = True
            break

    if not found:
        data.append({"name": target, "version": version})

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

    # 更新版本来源，CI 会基于这次提交构建产物。
    update_version_json(target, version)

    # 本地 Release 构建
    update_cmake_version(cmake_file, version)
    run(["cmake", "--build", str(BUILD_DIR), "--target", f"{target}_All", "--config", "Release"])

    # git commit & tag
    tag = f"{target}@{version}"
    run(
        [
            "git",
            "add",
            str(VERSION_JSON.relative_to(ROOT)),
            str(cmake_file.relative_to(ROOT)),
        ]
    )
    run(["git", "commit", "-m", tag])
    run(["git", "tag", tag])
    run(["git", "push"])
    run(["git", "push", "--tags"])

    print(f"\nDone. Pushed tag {tag}. GitHub Action will build and release.")


if __name__ == "__main__":
    main()
