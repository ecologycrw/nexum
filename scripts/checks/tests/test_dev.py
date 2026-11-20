import importlib.util
import json
import unittest
from pathlib import Path

import support  # noqa: F401

ROOT = Path(__file__).resolve().parents[3]


def load_dev():
    spec = importlib.util.spec_from_file_location("nexum_dev", ROOT / "scripts" / "dev.py")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


dev = load_dev()


class PresetTests(unittest.TestCase):
    def configure_presets(self) -> set[str]:
        presets = json.loads((ROOT / "CMakePresets.json").read_text(encoding="utf-8"))
        return {preset["name"] for preset in presets["configurePresets"] if not preset.get("hidden")}

    def test_each_system_gets_its_own_presets(self) -> None:
        self.assertEqual(dev.system_name("Windows"), "windows")
        self.assertEqual(dev.system_name("Linux"), "linux")
        self.assertEqual(dev.system_name("Darwin"), "macos")

    def test_every_preset_the_script_asks_for_exists(self) -> None:
        existing = self.configure_presets()
        wanted = {dev.preset_name(system, "default", kind)
                  for system in ("windows", "linux", "macos") for kind in ("dev", "release")}
        wanted |= {dev.preset_name(system, "default", "asan") for system in ("windows", "linux")
                   if dev.has_sanitizers(system, "default")}
        wanted.add(dev.preset_name("linux", "gcc", "dev"))
        self.assertEqual(wanted - existing, set())

    def test_every_preset_has_a_build_and_a_test_preset(self) -> None:
        presets = json.loads((ROOT / "CMakePresets.json").read_text(encoding="utf-8"))
        for kind in ("buildPresets", "testPresets"):
            with self.subTest(kind=kind):
                names = {preset["name"] for preset in presets[kind] if not preset.get("hidden")}
                self.assertEqual(self.configure_presets() - names, set())

    def test_the_player_presets_turn_the_authoring_mode_off(self) -> None:
        presets = json.loads((ROOT / "CMakePresets.json").read_text(encoding="utf-8"))
        player = next(preset for preset in presets["configurePresets"] if preset["name"] == "player")
        self.assertEqual(player["cacheVariables"]["NEXUM_AUTHORING"], "OFF")
        self.assertEqual(player["cacheVariables"]["CMAKE_BUILD_TYPE"], "Release")
        for system in ("windows", "linux", "macos"):
            release = next(preset for preset in presets["configurePresets"] if preset["name"] == f"{system}-release")
            self.assertIn("player", release["inherits"])

    def test_gcc_is_used_only_on_linux_for_the_dev_build(self) -> None:
        self.assertEqual(dev.preset_name("linux", "gcc", "dev"), "linux-gcc-dev")
        for system, kind in (("windows", "dev"), ("macos", "dev"), ("linux", "asan"), ("linux", "release")):
            with self.subTest(system=system, kind=kind):
                with self.assertRaises(dev.StepFailed):
                    dev.preset_name(system, "gcc", kind)

    def test_the_sanitizers_run_only_where_they_have_a_preset(self) -> None:
        self.assertTrue(dev.has_sanitizers("windows", "default"))
        self.assertTrue(dev.has_sanitizers("linux", "default"))
        self.assertFalse(dev.has_sanitizers("macos", "default"))
        self.assertFalse(dev.has_sanitizers("linux", "gcc"))


if __name__ == "__main__":
    unittest.main()
