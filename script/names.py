import re
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
PLUGINS_DIR = ROOT / "plugins"


def plugin_cmake_files() -> list[Path]:
    """Return CMake files that declare a JUCE plugin target."""
    return sorted(
        path
        for path in PLUGINS_DIR.rglob("CMakeLists.txt")
        if "juce_add_plugin" in path.read_text(encoding="utf-8")
    )


def available_plugins() -> dict[str, Path]:
    plugins: dict[str, Path] = {}
    for path in plugin_cmake_files():
        text = path.read_text(encoding="utf-8")
        match = re.search(
            r"^\s*set\s*\(\s*PLUGIN_NAME\s+([A-Za-z0-9_]+)\s*\)", text, re.MULTILINE
        )
        if match:
            plugins[match.group(1)] = path
    return plugins


def resolve(target_name: str) -> str | None:
    """Validate a tag/plugin name against declared CMake plugin targets."""
    return target_name if target_name in available_plugins() else None


def cmake_file_for(target_name: str) -> Path | None:
    return available_plugins().get(target_name)
