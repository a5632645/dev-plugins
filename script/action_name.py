import sys

from names import folder_for, resolve


def parse_tag(tag: str) -> tuple[str, str]:
    """从 tag 字符串 'PluginName@version' 中解析插件名和版本。"""
    parts = tag.split("@")
    if len(parts) != 2 or not parts[0] or not parts[1]:
        print(
            f"Error: invalid tag format '{tag}'. Expected 'Name@version'",
            file=sys.stderr,
        )
        sys.exit(1)
    return parts[0], parts[1]


if __name__ == "__main__":
    tag = sys.argv[1] if len(sys.argv) > 1 else ""
    field = sys.argv[2] if len(sys.argv) > 2 else "name"
    name, version = parse_tag(tag)
    target = resolve(name)
    if not target:
        print(f"Error: unknown plugin name '{name}'", file=sys.stderr)
        sys.exit(1)
    if field == "name":
        print(target)
    elif field == "version":
        print(version)
    elif field == "folder":
        f = folder_for(target)
        if f is None:
            print(f"Error: folder not found for '{target}'", file=sys.stderr)
            sys.exit(1)
        print(f)
    else:
        print(
            f"Error: unknown field '{field}'. Expected 'name', 'version', or 'folder'",
            file=sys.stderr,
        )
        sys.exit(1)
