import os
import subprocess
from pathlib import Path

from SCons.Script import Import

Import("env")


RESOURCE_NAMES = {
    "index.html": "indexHtml",
    "script.js": "scriptJs",
    "styles.css": "stylesCss",
}


def get_cpp_define(name, default):
    defines = env.get("CPPDEFINES", [])
    for item in defines:
        if isinstance(item, tuple) and len(item) == 2 and item[0] == name:
            value = item[1]
            if isinstance(value, str):
                return value.strip('\\"')
            return str(value)
        if item == name:
            return default
    return default


def ensure_frontend_build(frontend_dir, dist_dir):
    npm_command = "npm.cmd" if os.name == "nt" else "npm"
    if not (frontend_dir / "node_modules").exists():
        subprocess.run([npm_command, "install"], cwd=frontend_dir, check=True)

    subprocess.run([npm_command, "run", "build"], cwd=frontend_dir, check=True)


def substitute_placeholders(content, placeholders):
    for key, value in placeholders.items():
        content = content.replace(f"__{key}__", value)
    return content


def to_header_array(binary_content):
    return ", ".join(f"0x{byte:02x}" for byte in binary_content)


def write_header(output_file, dist_dir, placeholders):
    output_file.parent.mkdir(parents=True, exist_ok=True)

    with output_file.open("w", encoding="utf-8") as header:
        header.write("#pragma once\n\n")
        header.write("#include <Arduino.h>\n")
        header.write("#include <stddef.h>\n\n")

        for resource_name, variable_name in RESOURCE_NAMES.items():
            resource_path = dist_dir / resource_name
            binary_content = resource_path.read_bytes()

            if resource_path.suffix in {".html", ".js", ".css"}:
                text_content = binary_content.decode("utf-8")
                binary_content = substitute_placeholders(text_content, placeholders).encode("utf-8")

            header.write(f"const uint8_t {variable_name}[] PROGMEM = {{{to_header_array(binary_content)}}};\n")
            header.write(f"const size_t {variable_name}_len = {len(binary_content)};\n\n")


def main():
    project_dir = Path(env["PROJECT_DIR"])
    frontend_dir = project_dir / "frontend"
    dist_dir = project_dir / "resources" / "dist"
    output_file = project_dir / "include" / "embedded_resources.h"

    placeholders = {
        "VERSION": get_cpp_define("APP_VERSION", "0.0.0-local"),
        "CHANNEL": get_cpp_define("APP_CHANNEL", "local"),
        "BOARD": get_cpp_define("APP_BOARD", "unknown-board"),
    }

    ensure_frontend_build(frontend_dir, dist_dir)
    write_header(output_file, dist_dir, placeholders)
    print(f"Embedded frontend resources generated at {output_file}")


main()