import datetime
import os
import subprocess

from SCons.Script import Import

Import("env")


def read_define(name, default):
    for item in reversed(env.get("CPPDEFINES", [])):
        if isinstance(item, tuple) and len(item) == 2 and item[0] == name:
            value = item[1]
            return value.strip('\\"') if isinstance(value, str) else str(value)
    return default


def read_channel():
    environment_name = env.get("PIOENV", "")
    if environment_name.endswith("-release"):
        return "stable"
    if environment_name.endswith("-dev"):
        return "dev"
    return read_define("APP_CHANNEL", "dev")


def run_git(args):
    return subprocess.check_output(["git", *args], stderr=subprocess.STDOUT).strip().decode("utf-8")


def sanitize_identifier(value):
    sanitized = []
    previous_dash = False

    for char in value:
        if char.isalnum() or char in (".", "_"):
            sanitized.append(char)
            previous_dash = False
        else:
            if not previous_dash:
                sanitized.append("-")
                previous_dash = True

    result = "".join(sanitized).strip("-")
    return result or "detached"


def compute_version(channel):
    try:
        short_hash = run_git(["rev-parse", "--short", "HEAD"])
        branch_name = sanitize_identifier(
            os.getenv("GITHUB_REF_NAME") or run_git(["rev-parse", "--abbrev-ref", "HEAD"])
        )
        timestamp = os.getenv("BUILD_TIMESTAMP")

        if channel == "stable":
            latest_tag = run_git(["describe", "--tags", "--abbrev=0", "--match", "v*.*.*"])
            return latest_tag

        if timestamp:
            return f"{branch_name}-{short_hash}-{timestamp}"
        return f"{branch_name}-{short_hash}"
    except (subprocess.CalledProcessError, FileNotFoundError):
        return "v0.1.0-dev" if channel == "dev" else "v0.1.0"


channel = read_channel()
version = compute_version(channel)

env.AppendUnique(
    CPPDEFINES=[
        ("APP_VERSION", '\\"{}\\"'.format(version)),
    ]
)

print("Building mylamp version:", version, "channel:", channel)