#!/usr/bin/env python3
"""Shared build/test/package logic."""

import argparse
import shutil
import subprocess
import sys
import tempfile
from dataclasses import dataclass, field
from pathlib import Path
from typing import Dict, List, Optional


@dataclass
class PlatformConfig:
    build_dir: str
    generator: Optional[str]
    cmake_options: List[str] = field(default_factory=list)
    build_targets: List[str] = field(default_factory=list)
    needs_msvc_env: bool = False


PLATFORMS: Dict[str, PlatformConfig] = {
    "windows": PlatformConfig(
        build_dir="cmake-build-debug",
        generator=None,
        cmake_options=[],
        build_targets=[],
        needs_msvc_env=True,
    ),
    "linux": PlatformConfig(
        build_dir="cmake-build-debug-linux",
        generator="Ninja",
        cmake_options=["-DBUILD_CLIENT=OFF", "-DBUILD_TESTING=OFF"],
        build_targets=["fps_server"],
        needs_msvc_env=False,
    ),
}

DEFAULT_VCVARS = (
    r"C:\Program Files\Microsoft Visual Studio\2022\Community"
    r"\VC\Auxiliary\Build\vcvars64.bat"
)

PACKAGE_DLLS = [
    "SDL3.dll",
    "GameNetworkingSockets.dll",
    "abseil_dll.dll",
    "libcrypto-3-x64.dll",
    "libprotobuf.dll",
]


def project_root() -> Path:
    return Path(__file__).resolve().parent.parent


def build_type_from_args(release: bool) -> str:
    return "Release" if release else "Debug"


def normalize_zip_name(name: str) -> str:
    if name.lower().endswith(".zip"):
        return name
    return name + ".zip"


def msvc_environment(vcvars_path: str) -> Dict[str, str]:
    path = Path(vcvars_path)
    if not path.exists():
        print(f"[build] ERROR: Could not find vcvars64.bat at {vcvars_path}")
        sys.exit(1)

    result = subprocess.run(
        ["cmd", "/c", str(path), "&&", "set"],
        capture_output=True,
        text=True,
    )
    if result.returncode != 0:
        print("[build] ERROR: Failed to source vcvars64.bat")
        sys.exit(1)

    env: Dict[str, str] = {}
    for line in result.stdout.splitlines():
        key, sep, value = line.partition("=")
        if sep:
            env[key] = value
    return env


def run(cmd: List[str], env: Optional[Dict[str, str]] = None, prefix: str = "build") -> None:
    print(f"[{prefix}] {' '.join(cmd)}")
    result = subprocess.run(cmd, cwd=project_root(), env=env)
    if result.returncode != 0:
        print(f"[{prefix}] Command failed: {' '.join(cmd)}")
        sys.exit(result.returncode)


def configure_cmd(
    build_dir: str,
    build_type: str,
    generator: Optional[str] = None,
    cmake_options: Optional[List[str]] = None,
) -> List[str]:
    cmd = ["cmake", "-S", ".", "-B", build_dir]
    if generator:
        cmd += ["-G", generator]
    cmd += [f"-DCMAKE_BUILD_TYPE={build_type}"]
    cmd += cmake_options or []
    return cmd


def build_cmd(
    build_dir: str,
    build_type: str,
    generator: Optional[str],
    targets: Optional[List[str]] = None,
) -> List[str]:
    cmd = ["cmake", "--build", build_dir]
    for target in targets or []:
        cmd += ["--target", target]
    # Multi-config generators (the Windows/VS default) need --config; Ninja is
    # single-config and ignores/rejects it.
    if generator is None:
        cmd += ["--config", build_type]
    return cmd


def env_for_platform(config: PlatformConfig, vcvars_path: str) -> Optional[Dict[str, str]]:
    if config.needs_msvc_env:
        return msvc_environment(vcvars_path)
    return None


def cmd_build(platform: str, release: bool, vcvars_path: str) -> None:
    config = PLATFORMS[platform]
    build_type = build_type_from_args(release)
    env = env_for_platform(config, vcvars_path)

    print(f"[build] Configuring CMake {build_type}...")
    run(
        configure_cmd(config.build_dir, build_type, config.generator, config.cmake_options),
        env=env,
        prefix="build",
    )

    print("[build] Building...")
    run(
        build_cmd(config.build_dir, build_type, config.generator, config.build_targets),
        env=env,
        prefix="build",
    )

    exe = "fps_server" if platform == "linux" else "fps_demo.exe"
    print(f"[build] Done. Executable: {config.build_dir}/{exe}")


def cmd_test(platform: str, vcvars_path: str) -> None:
    config = PLATFORMS[platform]
    env = env_for_platform(config, vcvars_path)

    run(
        configure_cmd(
            "cmake-build-debug",
            "Debug",
            generator="Ninja",
            cmake_options=["-DBUILD_TESTING=ON"],
        ),
        env=env,
        prefix="test",
    )
    run(
        build_cmd("cmake-build-debug", "Debug", generator="Ninja", targets=["fps_demo_tests"]),
        env=env,
        prefix="test",
    )
    run(
        ["ctest", "--test-dir", "cmake-build-debug", "--output-on-failure"],
        env=env,
        prefix="test",
    )


def cmd_package(platform: str, vcvars_path: str, zip_name: Optional[str]) -> None:
    root = project_root()
    config = PLATFORMS[platform]
    out_dir = root / "release"

    cmd_build(platform, release=True, vcvars_path=vcvars_path)

    print(f"[package] Assembling {out_dir}...")
    if out_dir.exists():
        shutil.rmtree(out_dir)
    out_dir.mkdir(parents=True)

    build_dir = root / config.build_dir
    exe_src = build_dir / "fps_demo.exe"
    if not exe_src.exists():
        print(f"[package] ERROR: fps_demo.exe not found in {config.build_dir}.")
        sys.exit(1)
    shutil.copy2(exe_src, out_dir / exe_src.name)

    for dll in PACKAGE_DLLS:
        dll_src = build_dir / dll
        if dll_src.exists():
            shutil.copy2(dll_src, out_dir / dll)

    shutil.copytree(root / "shaders", out_dir / "shaders", dirs_exist_ok=True)
    shutil.copytree(root / "assets", out_dir / "assets", dirs_exist_ok=True)
    shutil.copy2(root / "allow_firewall.bat", out_dir / "allow_firewall.bat")

    if zip_name:
        normalized = normalize_zip_name(zip_name)
        zip_path = out_dir / normalized
        if zip_path.exists():
            zip_path.unlink()
        print(f"[package] Zipping to {zip_path}...")
        # Archive to a temp location outside out_dir first: zipping out_dir
        # while writing the zip into out_dir would make the archive include
        # (and keep growing to include) itself.
        with tempfile.TemporaryDirectory() as tmp_dir:
            archive_base = str(Path(tmp_dir) / zip_path.stem)
            archive_path = shutil.make_archive(archive_base, "zip", root_dir=out_dir)
            shutil.move(archive_path, zip_path)

    print(f"[package] Done. Release folder: {out_dir}")


def build_arg_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Build/test/package the shooter project.")
    subparsers = parser.add_subparsers(dest="command", required=True)

    build_parser = subparsers.add_parser("build", help="Configure + build.")
    build_parser.add_argument(
        "--platform", required=True, choices=PLATFORMS.keys(),
        help="Target platform; selects build dir, generator, and cmake options.",
    )
    build_parser.add_argument(
        "release", nargs="?", default=None, choices=["release"],
        help="Pass 'release' to build Release instead of Debug (default: Debug).",
    )
    build_parser.add_argument(
        "--vcvars", default=DEFAULT_VCVARS,
        help="Path to vcvars64.bat, used to set up the MSVC environment on Windows "
        f"(default: {DEFAULT_VCVARS}).",
    )

    test_parser = subparsers.add_parser("test", help="Configure with tests + build + run ctest.")
    test_parser.add_argument(
        "--platform", required=True, choices=PLATFORMS.keys(),
        help="Target platform; selects build dir, generator, and cmake options.",
    )
    test_parser.add_argument(
        "--vcvars", default=DEFAULT_VCVARS,
        help="Path to vcvars64.bat, used to set up the MSVC environment on Windows "
        f"(default: {DEFAULT_VCVARS}).",
    )

    package_parser = subparsers.add_parser("package", help="Build release + assemble release/.")
    package_parser.add_argument(
        "--platform", required=True, choices=PLATFORMS.keys(),
        help="Target platform; selects build dir, generator, and cmake options.",
    )
    package_parser.add_argument(
        "zip_name", nargs="?", default=None,
        help="Optional name for a zip archive of release/ (.zip appended if missing). "
        "If omitted, no zip is created.",
    )
    package_parser.add_argument(
        "--vcvars", default=DEFAULT_VCVARS,
        help="Path to vcvars64.bat, used to set up the MSVC environment on Windows "
        f"(default: {DEFAULT_VCVARS}).",
    )

    return parser


def main(argv: Optional[List[str]] = None) -> int:
    parser = build_arg_parser()
    args = parser.parse_args(argv)

    if args.command == "build":
        is_release = bool(args.release) and args.release.lower() == "release"
        cmd_build(args.platform, is_release, args.vcvars)
    elif args.command == "test":
        cmd_test(args.platform, args.vcvars)
    elif args.command == "package":
        cmd_package(args.platform, args.vcvars, args.zip_name)

    return 0


if __name__ == "__main__":
    sys.exit(main())
