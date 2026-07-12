"""列出所有可发布的插件（名称 + 版本号）。"""

import re

from names import available_plugins


def main():
    for name, cmake_path in sorted(available_plugins().items()):
        text = cmake_path.read_text(encoding="utf-8")
        m = re.search(
            r"^\s*set\s*\(\s*PLUGIN_VERSION\s+([^)\s]+)\s*\)", text, re.MULTILINE
        )
        version = m.group(1) if m else "?"
        print(f"  {name}  {version}")


if __name__ == "__main__":
    main()
