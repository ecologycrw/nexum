"""Keeps the environment of the AI consistent with the rules written in AGENTS.md."""

from __future__ import annotations

import json
import re
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts" / "checks"))

from nexum_checks import pr_summary  # noqa: E402

SKILLS = ("situacao", "planejar", "implementar", "verificar", "revisar", "fechar", "retrospectiva", "consolidar",
          "propor", "consultar-central")
AGENTS = ("revisor", "auditor")
HOOKS = ("guard-bash", "guard-central", "session-start", "stop-check")

# Every protected path, as written in the list of AGENTS.md.
TRAVAS = (".claude/", ".githooks/", ".github/", "cmake/", "CMakePresets.json", "vcpkg.json", "vcpkg-configuration.json",
          ".clang-format", ".clang-tidy", ".editorconfig", "AGENTS.md", "CLAUDE.md", "scripts/dev.py", "scripts/checks/",
          "tests/architecture/", "tests/hooks/", "tests/visual/approved/", "docs/referencia/")
OWNER_DOCUMENTS = ("docs/VISION.md", "docs/ROADMAP.md", "docs/VISUAL.md")
SHELLS = ("Bash", "PowerShell")


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8-sig")


def settings() -> dict:
    return json.loads(read(".claude/settings.json"))


class SkillTests(unittest.TestCase):
    def test_every_skill_of_the_cycle_exists_with_its_own_name(self) -> None:
        for skill in SKILLS:
            with self.subTest(skill=skill):
                self.assertRegex(read(f".claude/skills/{skill}/SKILL.md"), rf"(?m)^name: {skill}$")

    def test_every_skill_has_a_description(self) -> None:
        for skill in SKILLS:
            with self.subTest(skill=skill):
                self.assertRegex(read(f".claude/skills/{skill}/SKILL.md"), r"(?m)^description: .{20,}$")

    def test_the_rules_mention_every_skill(self) -> None:
        for skill in SKILLS:
            with self.subTest(skill=skill):
                self.assertIn(f"/{skill}", read("CLAUDE.md"))

    def test_the_skills_use_the_commands_of_this_repository(self) -> None:
        for skill in SKILLS:
            with self.subTest(skill=skill):
                self.assertNotIn("tools/dev.py", read(f".claude/skills/{skill}/SKILL.md"))
                self.assertNotIn("tools/checks", read(f".claude/skills/{skill}/SKILL.md"))

    def test_the_skill_of_the_central_never_points_to_the_code_of_the_mumain(self) -> None:
        text = read(".claude/skills/consultar-central/SKILL.md")
        self.assertIn("nunca é lido", text)
        self.assertNotRegex(text, r"<path>/Client/(?!`)")


class AgentTests(unittest.TestCase):
    def test_the_agents_exist_and_cannot_edit_files(self) -> None:
        for agent in AGENTS:
            with self.subTest(agent=agent):
                text = read(f".claude/agents/{agent}.md")
                self.assertRegex(text, rf"(?m)^name: {agent}$")
                tools = re.search(r"(?m)^tools: (.+)$", text).group(1)
                self.assertNotRegex(tools, r"\b(Edit|Write|NotebookEdit)\b")

    def test_the_reviewer_knows_every_invariant(self) -> None:
        highest = max(re.findall(r"INV-(\d{2})", read("docs/ARCHITECTURE.md")))
        self.assertIn(f"INV-01 a INV-{highest}", read(".claude/agents/revisor.md"))

    def test_the_reviewer_checks_the_clean_room(self) -> None:
        self.assertIn("Sala limpa", read(".claude/agents/revisor.md"))


class HookTests(unittest.TestCase):
    def commands(self) -> list[str]:
        return [hook["command"] for groups in settings()["hooks"].values() for group in groups for hook in group["hooks"]]

    def test_every_hook_is_connected(self) -> None:
        for hook in HOOKS:
            with self.subTest(hook=hook):
                self.assertTrue((ROOT / f".claude/hooks/{hook}.ps1").is_file())
                self.assertTrue(any(f"{hook}.ps1" in command for command in self.commands()))

    def test_hooks_are_saved_with_a_byte_order_mark(self) -> None:
        for path in [f".claude/hooks/{hook}.ps1" for hook in HOOKS] + [
                str(script.relative_to(ROOT)) for script in sorted((ROOT / "tests" / "hooks").glob("*.ps1"))]:
            with self.subTest(path=path):
                self.assertTrue((ROOT / path).read_bytes().startswith(b"\xef\xbb\xbf"))

    def test_the_central_guard_watches_every_tool_that_reaches_files(self) -> None:
        groups = settings()["hooks"]["PreToolUse"]
        matcher = next(group["matcher"] for group in groups if "guard-central" in group["hooks"][0]["command"])
        for tool in ("Read", "Grep", "Glob", "Edit", "Write", "NotebookEdit", "Bash", "PowerShell"):
            with self.subTest(tool=tool):
                self.assertIn(tool, matcher.split("|"))

    def test_the_central_guard_blocks_the_code_of_the_mumain(self) -> None:
        text = read(".claude/hooks/guard-central.ps1")
        self.assertIn("$script:CleanRoomPart = 'client'", text)
        self.assertNotRegex(text, r"\$script:ReadableParts = @\([^)]*'client'")

    def test_the_path_of_the_central_stays_out_of_git(self) -> None:
        self.assertIn(".claude/central.local.json", read(".gitignore"))


class PermissionTests(unittest.TestCase):
    def test_dangerous_commands_are_denied_in_both_shells(self) -> None:
        deny = settings()["permissions"]["deny"]
        for shell in SHELLS:
            for command in ("git push --force:*", "git commit --no-verify:*", "git config:*", "gh pr merge:*",
                            "gh auth:*", "gh api:*", "git reset --hard:*", "git rebase:*"):
                rule = f"{shell}({command})"
                with self.subTest(rule=rule):
                    self.assertIn(rule, deny)
        self.assertIn("Read(~/.ssh/**)", deny)

    def test_editing_a_lock_asks_the_owner(self) -> None:
        ask = settings()["permissions"]["ask"]
        for path in (*TRAVAS, *OWNER_DOCUMENTS):
            rule = f"Edit({path}**)" if path.endswith("/") else f"Edit({path})"
            with self.subTest(rule=rule):
                self.assertIn(rule, ask)

    def test_a_merge_always_asks_the_owner(self) -> None:
        permissions = settings()["permissions"]
        for shell in SHELLS:
            with self.subTest(shell=shell):
                self.assertIn(f"{shell}(git merge:*)", permissions["ask"])
                self.assertFalse(any("git merge" in rule for rule in permissions["allow"]))

    def test_the_daily_git_commands_of_the_skills_do_not_ask_the_owner(self) -> None:
        allow = settings()["permissions"]["allow"]
        for shell in SHELLS:
            for command in ("git push", "git switch main", "git pull --ff-only", "git status:*", "git commit:*"):
                rule = f"{shell}({command})"
                with self.subTest(rule=rule):
                    self.assertIn(rule, allow)

    def test_the_settings_have_no_personal_paths(self) -> None:
        self.assertNotRegex(read(".claude/settings.json"), r"(?i)c:[/\\]+users|/home/")


class LockListTests(unittest.TestCase):
    def test_agents_md_lists_every_lock(self) -> None:
        for path in TRAVAS:
            with self.subTest(path=path):
                self.assertIn(f"`{path}`", read("AGENTS.md"))

    def test_agents_md_lists_every_document_of_the_owner(self) -> None:
        for path in OWNER_DOCUMENTS:
            with self.subTest(path=path):
                self.assertIn(f"`{path}`", read("AGENTS.md"))

    def test_code_owners_lists_every_lock(self) -> None:
        for path in (*TRAVAS, *OWNER_DOCUMENTS):
            with self.subTest(path=path):
                self.assertRegex(read(".github/CODEOWNERS"), rf"(?m)^/{re.escape(path)}\s")

    def test_the_pull_request_summary_flags_every_lock(self) -> None:
        for path in (*TRAVAS, *OWNER_DOCUMENTS):
            sample = f"{path}file" if path.endswith("/") else path
            with self.subTest(path=sample):
                self.assertTrue(pr_summary.is_protected(sample))

    def test_the_terminal_guard_protects_every_lock(self) -> None:
        guard = read(".claude/hooks/guard-bash.ps1")
        for fragment in ("scripts[/\\\\]dev\\.py", "scripts[/\\\\]checks", "docs[/\\\\]referencia",
                         "tests[/\\\\]visual[/\\\\]approved", "tests[/\\\\]hooks", "tests[/\\\\]architecture"):
            with self.subTest(fragment=fragment):
                self.assertIn(fragment, guard)


if __name__ == "__main__":
    unittest.main()
