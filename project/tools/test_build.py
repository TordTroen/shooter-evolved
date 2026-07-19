import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import build  # noqa: E402


class PlatformConfigToCommandTests(unittest.TestCase):
    def test_windows_configure_has_no_generator_or_extra_options(self):
        config = build.PLATFORMS["windows"]
        cmd = build.configure_cmd(config.build_dir, "Debug", config.generator, config.cmake_options)
        self.assertNotIn("-G", cmd)
        self.assertEqual(cmd, ["cmake", "-S", ".", "-B", "cmake-build-debug", "-DCMAKE_BUILD_TYPE=Debug"])

    def test_windows_build_targets_all(self):
        config = build.PLATFORMS["windows"]
        cmd = build.build_cmd(config.build_dir, "Debug", config.generator, config.build_targets)
        self.assertNotIn("--target", cmd)

    def test_linux_configure_has_ninja_and_client_test_flags(self):
        config = build.PLATFORMS["linux"]
        cmd = build.configure_cmd(config.build_dir, "Debug", config.generator, config.cmake_options)
        self.assertEqual(
            cmd,
            [
                "cmake", "-S", ".", "-B", "cmake-build-debug-linux",
                "-G", "Ninja",
                "-DCMAKE_BUILD_TYPE=Debug",
                "-DBUILD_CLIENT=OFF",
                "-DBUILD_TESTING=OFF",
            ],
        )

    def test_linux_build_targets_fps_server(self):
        config = build.PLATFORMS["linux"]
        cmd = build.build_cmd(config.build_dir, "Debug", config.generator, config.build_targets)
        self.assertIn("fps_server", cmd)


class BuildTypeSelectionTests(unittest.TestCase):
    def test_no_arg_is_debug(self):
        self.assertEqual(build.build_type_from_args(False), "Debug")

    def test_release_arg_is_release(self):
        self.assertEqual(build.build_type_from_args(True), "Release")


class ConfigGatingTests(unittest.TestCase):
    def test_config_flag_present_for_windows_multi_config(self):
        cmd = build.build_cmd("cmake-build-debug", "Release", generator=None)
        self.assertIn("--config", cmd)
        self.assertIn("Release", cmd)

    def test_config_flag_absent_for_ninja(self):
        cmd = build.build_cmd("cmake-build-debug-linux", "Release", generator="Ninja")
        self.assertNotIn("--config", cmd)


class ZipNameNormalizationTests(unittest.TestCase):
    def test_appends_zip_suffix(self):
        self.assertEqual(build.normalize_zip_name("foo"), "foo.zip")

    def test_leaves_existing_zip_suffix(self):
        self.assertEqual(build.normalize_zip_name("foo.zip"), "foo.zip")

    def test_suffix_check_is_case_insensitive(self):
        self.assertEqual(build.normalize_zip_name("foo.ZIP"), "foo.ZIP")


class ProjectRootResolutionTests(unittest.TestCase):
    def test_project_root_is_tools_parent(self):
        self.assertEqual(build.project_root(), Path(__file__).resolve().parent.parent)


class MsvcEnvironmentTests(unittest.TestCase):
    def test_missing_vcvars_path_exits_non_zero(self):
        with self.assertRaises(SystemExit) as ctx:
            build.msvc_environment(str(Path(__file__).resolve().parent / "no_such_vcvars.bat"))
        self.assertNotEqual(ctx.exception.code, 0)


if __name__ == "__main__":
    unittest.main()
