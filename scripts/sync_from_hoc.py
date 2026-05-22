#!/usr/bin/env python3
"""
Smart HOC -> Ryazha-CLK Sync Script

Pulls commits from upstream Horizon-OC repo, adapts paths/names to our
Ryazha-CLK conventions, and creates clean commits without exposing HOC origin.

Features:
- Filename adaptation: hoc-clk -> ryazha-clk, hocclk -> rclk
- Content adaptation: HOC namespaces and symbols renamed
- Protected paths: skips Ryazha-Авто and VRR-related files
- Excludes: vrr, Horizon-OC-Monitor, deleted/renamed-by-us paths

Usage:
    python3 sync_from_hoc.py [--dry-run] [--since DAYS_BACK]
"""

import argparse
import os
import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path
from typing import Iterable

# Ensure UTF-8 output even on Windows consoles
if hasattr(sys.stdout, "reconfigure"):
    try:
        sys.stdout.reconfigure(encoding="utf-8", errors="replace")
        sys.stderr.reconfigure(encoding="utf-8", errors="replace")
    except Exception:
        pass

UPSTREAM_REPO = "https://github.com/Horizon-OC/Horizon-OC.git"
UPSTREAM_BRANCH = "main"

# ──────────────────────────────────────────────────────────────────────
# Path adaptation: HOC path -> our path
# ──────────────────────────────────────────────────────────────────────
# Когда upstream трогает hoc-clk/* мы переписываем путь на наш ryazha-clk/*.
# ВНИМАНИЕ: после adapt_path() ещё проверяется is_protected/is_excluded --
# даже если путь "перенесли", мы можем его не записывать (см. ниже).
PATH_MAPPINGS = [
    ("Source/hoc-clk/", "Source/ryazha-clk/"),
    ("Source/Horizon-OC-Monitor/", "Source/Ryazha-Status-Monitor/"),
    ("dist/config/horizon-oc/", "dist/config/ryazha-clk/"),
    ("common/include/hocclk/", "common/include/rclk/"),
    ("common/include/hocclk.h", "common/include/rclk.h"),
    # Иногда upstream хранит вспомогательные файлы в hocclk/ (нижний регистр)
    ("hocclk/", "rclk/"),
]

# ──────────────────────────────────────────────────────────────────────
# Content adaptation: token -> replacement (regex)
# ──────────────────────────────────────────────────────────────────────
# Длинные паттерны раньше коротких, чтобы "Horizon-OC" не поломалось от "horizon-oc".
# Применяется ТОЛЬКО к текстовым файлам (см. is_text_path).
CONTENT_REPLACEMENTS = [
    # ── commit-message-style prefix ──
    (r"\bhocclk:\s*",  "ryazha-clk: "),
    (r"\bhoc-clk:\s*", "ryazha-clk: "),

    # ── namespace/include renames ──
    (r"hocclk/",         "rclk/"),
    (r"\"hocclk\.h\"",   "\"rclk.h\""),
    (r"<hocclk\.h>",     "<rclk.h>"),
    (r"\"hocclk\.hpp\"", "\"rclk.hpp\""),
    (r"<hocclk\.hpp>",   "<rclk.hpp>"),

    # ── symbol renames (длинные раньше коротких!) ──
    (r"\bHOCCLK_",       "RCLK_"),
    (r"\bHocClkGov\b",   "RClkGov"),
    (r"\bHocClk\b",      "RClk"),
    (r"\bHOCClk\b",      "RCLK"),
    (r"\bHOC_",          "RCLK_"),
    (r"\bhocclkGetVersionString\b", "rclkGetVersionString"),
    (r"\bhocclkGetConfigValues\b",  "rclkGetConfigValues"),
    (r"\bhocclkSetConfigValues\b",  "rclkSetConfigValues"),
    # Generic IPC fallback -- любой hocclk* идентификатор.
    (r"\bhocclk([A-Z]\w*)\b", r"rclk\1"),

    # ── build artifact names ──
    (r"\bhoc-clk\.nsp\b", "ryazha-clk.nsp"),
    (r"\bhoc-clk\.ovl\b", "ryazha-clk.ovl"),
    (r"\bhoc-clk\.kip\b", "rcu.kip"),
    (r"\bhoc\.kip\b",     "rcu.kip"),
    (r"\bhoc-clk\.nro\b", "ryazha-clk.nro"),
    (r"\bhoc-clk\.elf\b", "ryazha-clk.elf"),

    # ── config paths ──
    (r"\bhorizon-oc/\b",  "ryazha-clk/"),
    (r"/config/horizon-oc", "/config/ryazha-clk"),
    (r"\[horizon-oc\]",   "[ryazha-clk]"),
    (r"\[hoc-clk\]",      "[ryazha-clk]"),

    # ── repo URLs ──
    (r"github\.com/Horizon-OC/Horizon-OC",
     "github.com/Dimanchikgshehsbshene/RCU"),
    (r"github\.com/Horizon-OC/",
     "github.com/Dimanchikgshehsbshene/"),

    # ── user-facing strings ──
    (r"\bHorizon-OC\b",   "Ryazha-CLK"),
    (r"\bHorizon OC\b",   "Ryazha CLK"),
    (r"\bHorizonOC\b",    "RyazhaCLK"),
    (r"\bhorizon-oc\b",   "ryazha-clk"),
    (r"\bhorizonoc\b",    "ryazhaclk"),
    (r"\bHOC\b",          "RCLK"),   # standalone capital (e.g. "HOC sysmodule")
    (r"\bhoc\b",          "ryazha-clk"),
]

# ──────────────────────────────────────────────────────────────────────
# PROTECTED PATHS -- эти файлы написаны нами или существенно
# модифицированы. Никогда не overwrite-аем upstream'ом.
# ──────────────────────────────────────────────────────────────────────
PROTECTED_PATHS = [
    # ── наши уникальные фичи (Ryazha-Авто, VRR, living ladder) ──
    "auto_ryazha",
    "Ryazha-Status-Monitor",
    "living_ladder",
    "display_hz_trackbar",
    "display_refresh_rate",

    # ── наш i18n + переводы (переписаны полностью с нуля) ──
    "Source/ryazha-clk/overlay/src/i18n.cpp",
    "Source/ryazha-clk/overlay/src/i18n.hpp",
    "Source/ryazha-clk/overlay/lang/",                 # все переводы
    "Source/ryazha-clk/overlay/src/ui/gui/labels.cpp", # русские labels
    "Source/ryazha-clk/overlay/src/ui/gui/labels.h",
    "Source/ryazha-clk/overlay/src/ui/gui/language_gui.cpp",
    "Source/ryazha-clk/overlay/src/ui/gui/language_gui.h",

    # ── наш кастомный About / Info / Misc UI (упоминания автора и т.п.) ──
    "Source/ryazha-clk/overlay/src/ui/gui/about_gui.cpp",
    "Source/ryazha-clk/overlay/src/ui/gui/about_gui.h",
    "Source/ryazha-clk/overlay/src/ui/gui/info_gui.cpp",
    "Source/ryazha-clk/overlay/src/ui/gui/info_gui.h",
    "Source/ryazha-clk/overlay/src/ui/gui/misc_gui.cpp",
    "Source/ryazha-clk/overlay/src/ui/gui/misc_gui.h",
    "Source/ryazha-clk/overlay/src/ui/gui/cat.h",      # пиксельарт автора

    # ── manifest sysmodule (TitleID наш, perms кастомные) ──
    "Source/ryazha-clk/sysmodule/toolbox.json",
    "Source/ryazha-clk/sysmodule/perms.json",

    # ── билд-инфраструктура и upstream-mirror submodule'ы ──
    "Source/ryazha-clk/overlay/lib/libryazhahand",
    "Source/ryazha-clk/overlay/Makefile",  # переименован на ryazhahand.mk
    "Source/ryazha-clk/build.sh",
    "Source/ryazha-clk/bitmap.py",
    "Source/ryazha-clk/config.ini.template",
    "Source/ryazha-clk/MIGRATION.md",
    "Source/ryazha-clk/LICENSE",
    "Source/ryazha-clk/README.md",

    # ── корневая инфраструктура repo ──
    ".github/",   # workflows, FUNDING, templates -- мы держим свой набор
    "scripts/sync_from_hoc.py",
    "scripts/fix_atmosphere_loader_includes.py",
    "build.sh",
    "collect_dist.sh",
    "ams_ver.txt",
    "ams_patch.bat",
    "fix_head.sh",
    "README.md",
    "README_RU.md",
    "MIGRATION.md",
    "COMPILING.md",
    "CONTRIBUTING.md",
    "SECURITY.md",
    "CODE_OF_CONDUCT.md",
    "LICENSE",
    ".gitmodules",
    ".gitignore",
    ".gitattributes",
]

# ──────────────────────────────────────────────────────────────────────
# EXCLUDED PATHS -- целые подкаталоги, которые мы не синхронизируем
# (либо это сторонние submodule'ы, либо генерируемые артефакты).
# ──────────────────────────────────────────────────────────────────────
EXCLUDED_PATHS = [
    # ── upstream фичи, которые мы реализовали по-своему ──
    "vrr/",
    "Source/Horizon-OC-Monitor/",
    "Source/vrr/",

    # ── сторонние submodule'ы (синхронятся отдельным workflow) ──
    "Source/Atmosphere/",
    "Source/Atmosphere-Patches/",
    "Source/Old-Atmosphere-Patches/",
    "Source/SaltyNX/",
    "Source/MemTesterNX/",
    "Source/TinyMemBenchNX/",
    "Source/fatal_handler_payload/",

    # ── артефакты сборки ──
    "build/",
    "build__bak/",
    "dist/",
    "Source/ryazha-clk/dist/",
    "Source/ryazha-clk/overlay/build/",
    "Source/ryazha-clk/overlay/out/",
    "Source/ryazha-clk/sysmodule/build/",
    "Source/ryazha-clk/sysmodule/out/",

    # ── HOC-специфичные скрипты, которые не имеют смысла у нас ──
    "fix_head.sh",       # их рескайн-скрипт
    "ams_patch.bat",     # их Windows-only ams-патчер
]


def run(cmd: list[str], cwd: str | None = None, check: bool = True) -> str:
    """Run a shell command and return stdout decoded as UTF-8.
    Lossy on invalid bytes -- callers that need real binary data must
    use run_bytes() instead.
    """
    result = subprocess.run(
        cmd, cwd=cwd, check=check, capture_output=True
    )
    # Decode with replacement -- никогда не падаем из-за бинарного шума
    # в stdout (например, file-content git show на PNG/ICO).
    return result.stdout.decode("utf-8", errors="replace")


def run_bytes(cmd: list[str], cwd: str | None = None, check: bool = True) -> bytes:
    """Run a shell command and return stdout as raw bytes.
    Used для бинарных файлов (icons, kip, blobs) которые нельзя
    декодировать как текст без потерь."""
    result = subprocess.run(
        cmd, cwd=cwd, check=check, capture_output=True
    )
    return result.stdout


def _matches_pattern(path: str, pattern: str) -> bool:
    """Match path against pattern. Pattern ending with '/' = directory prefix.
    Otherwise full-path или basename match. Не используем raw 'in' --
    он матчит слишком широко (e.g. 'LICENSE' матчился бы в любом пути)."""
    if pattern.endswith("/"):
        # directory prefix match
        return path.startswith(pattern) or ("/" + pattern) in ("/" + path)
    # full path
    if path == pattern:
        return True
    # path относится к директории, заданной pattern
    if path.startswith(pattern + "/"):
        return True
    # basename match (для коротких имён типа LICENSE, .gitignore)
    if "/" not in pattern and os.path.basename(path) == pattern:
        return True
    return False


def is_protected(path: str) -> bool:
    """Check if a path is protected (our code, don't overwrite).
    Сравниваем с обоими -- сырым upstream-путём и адаптированным,
    чтобы защита работала независимо от направления match'а.
    """
    candidates = (path, adapt_path(path))
    return any(_matches_pattern(p, pat) for p in candidates for pat in PROTECTED_PATHS)


def is_excluded(path: str) -> bool:
    """Check if a path should be excluded from sync entirely."""
    candidates = (path, adapt_path(path))
    return any(_matches_pattern(p, pat) for p in candidates for pat in EXCLUDED_PATHS)


def adapt_path(path: str) -> str:
    """Transform HOC path to our path conventions."""
    for src, dst in PATH_MAPPINGS:
        if src in path:
            path = path.replace(src, dst)
    return path


def adapt_content(content: str) -> str:
    """Transform HOC content to our naming conventions."""
    for pattern, repl in CONTENT_REPLACEMENTS:
        content = re.sub(pattern, repl, content)
    return content


def clone_upstream(target_dir: Path) -> None:
    """Clone the upstream HOC repo."""
    print(f"[*] Cloning upstream {UPSTREAM_REPO} ...")
    run(["git", "clone", "--depth=50", "--branch", UPSTREAM_BRANCH,
         UPSTREAM_REPO, str(target_dir)])


def get_recent_commits(repo_dir: Path, since_days: int) -> list[str]:
    """Get commit SHAs from the last N days."""
    out = run(
        ["git", "log", f"--since={since_days} days ago", "--format=%H", "--reverse"],
        cwd=str(repo_dir),
    )
    return [line.strip() for line in out.splitlines() if line.strip()]


def get_changed_files(repo_dir: Path, commit: str) -> list[tuple[str, str]]:
    """Get files changed in a commit. Returns (status, path) tuples."""
    out = run(
        ["git", "show", "--name-status", "--format=", commit],
        cwd=str(repo_dir),
    )
    files = []
    for line in out.splitlines():
        line = line.strip()
        if not line:
            continue
        parts = line.split("\t")
        if len(parts) >= 2:
            files.append((parts[0], parts[-1]))
    return files


def get_file_content_at(repo_dir: Path, commit: str, path: str) -> bytes | None:
    """Get the content of a file at a specific commit as RAW BYTES.
    Текст-decoding делает только caller'у и только для известных
    текстовых расширений. Иначе UnicodeDecodeError при синке бинарей
    (PNG-иконки, KIP, ELF).
    """
    try:
        return run_bytes(
            ["git", "show", f"{commit}:{path}"],
            cwd=str(repo_dir),
            check=True,
        )
    except subprocess.CalledProcessError:
        return None


# Расширения, которые однозначно текстовые. Только их пропускаем через
# adapt_content() и пишем как UTF-8. Всё остальное -- write_bytes.
TEXT_EXTENSIONS: tuple[str, ...] = (
    ".cpp", ".hpp", ".h", ".c", ".cc", ".cxx", ".inc",
    ".md", ".txt", ".json", ".yml", ".yaml",
    ".sh", ".bat", ".ps1",
    ".ini", ".cfg", ".conf", ".toml",
    ".py", ".rb", ".lua",
    ".mk", ".cmake",
    ".gitignore", ".gitattributes", ".gitmodules",
    ".clang-format", ".clang-tidy", ".editorconfig",
    ".html", ".css", ".js",
)

# Дополнительные текстовые имена без расширения.
TEXT_BASENAMES: tuple[str, ...] = (
    "Makefile", "LICENSE", "AUTHORS", "CHANGELOG", "README",
    "Dockerfile", "Doxyfile", ".clang-format", ".clang-tidy",
    ".editorconfig", ".gitignore", ".gitattributes", ".gitmodules",
)


def is_text_path(path: str) -> bool:
    lower = path.lower()
    if lower.endswith(TEXT_EXTENSIONS):
        return True
    basename = os.path.basename(path)
    return basename in TEXT_BASENAMES or any(basename.startswith(b) for b in TEXT_BASENAMES)


def get_commit_message(repo_dir: Path, commit: str) -> tuple[str, str]:
    """Get commit subject and body."""
    subject = run(
        ["git", "log", "-1", "--format=%s", commit], cwd=str(repo_dir)
    ).strip()
    body = run(
        ["git", "log", "-1", "--format=%b", commit], cwd=str(repo_dir)
    ).strip()
    return subject, body


def commit_should_be_skipped(subject: str, body: str, files: list[tuple[str, str]]) -> str | None:
    """Decide if a commit should be SKIPPED ENTIRELY.

    NB: protected-paths больше не повод отбрасывать весь коммит --
    apply_commit() per-file пропускает protected'ы. Отбрасываем
    только если в коммите ВООБЩЕ нет file changes для нас.
    """
    text = f"{subject}\n{body}".lower()

    # Skip merges
    if subject.lower().startswith("merge "):
        return "merge commit"

    # Если все файлы либо excluded, либо protected -- ничего не вытащим, скип.
    # is_protected() и is_excluded() уже проверяют path+adapt_path внутри,
    # передаём сырой upstream-путь.
    actionable = [p for _, p in files if not is_excluded(p) and not is_protected(p)]
    if not actionable:
        return "no actionable files (all excluded/protected)"

    # Топик-конфликт: коммит про наши фичи -- скорее всего поломает.
    if re.search(r"\bvrr\b|\bryazha\b|ryazha-авто|auto[\s_-]?ryazha", text):
        return "topic conflicts with our Ryazha/VRR features"

    return None


def apply_commit(our_repo: Path, upstream_repo: Path, commit: str, dry_run: bool) -> bool:
    """Apply a single HOC commit to our repo with path/content adaptation."""
    subject, body = get_commit_message(upstream_repo, commit)
    files = get_changed_files(upstream_repo, commit)

    skip_reason = commit_should_be_skipped(subject, body, files)
    if skip_reason:
        print(f"[-] Skip {commit[:7]}: {skip_reason}")
        return False

    print(f"[+] Apply {commit[:7]}: {subject}")
    if dry_run:
        for status, path in files:
            if is_excluded(path):
                print(f"    SKIP-EX {path}")
                continue
            if is_protected(path):
                print(f"    SKIP-PR {path}")
                continue
            adapted = adapt_path(path)
            marker = "TEXT" if is_text_path(path) else "BIN "
            arrow = " -> " if adapted != path else "    "
            print(f"    {status} {marker} {path}{arrow}{adapted if adapted != path else ''}")
        return True

    # Apply each file change
    applied_any = False
    for status, path in files:
        if is_excluded(path):
            print(f"    (excluded {path})")
            continue
        if is_protected(path):
            print(f"    (protected {path} -- keeping our version)")
            continue
        our_path = our_repo / adapt_path(path)

        if status.startswith("D"):
            if our_path.exists():
                our_path.unlink()
                applied_any = True
            continue

        raw = get_file_content_at(upstream_repo, commit, path)
        if raw is None:
            continue

        our_path.parent.mkdir(parents=True, exist_ok=True)

        # Текстовые файлы -- адаптируем имена/пути внутри + пишем UTF-8.
        # Всё остальное (PNG, ICO, KIP, ELF, BIN) -- write_bytes как есть,
        # никакого regex'а по байтам.
        if is_text_path(path):
            try:
                text = raw.decode("utf-8")
            except UnicodeDecodeError:
                # Случай: файл с похожим текстовым расширением, но
                # содержит non-UTF-8 байты (legacy CP-1251 и т.п.).
                # Лучше пропустить с предупреждением, чем повредить
                # данные адаптацией по битому декоду.
                print(f"    (skip {path}: non-UTF8 text -- saving raw bytes)")
                our_path.write_bytes(raw)
            else:
                text = adapt_content(text)
                our_path.write_text(text, encoding="utf-8")
        else:
            our_path.write_bytes(raw)

        applied_any = True

    if not applied_any:
        return False

    # Make a clean commit without HOC attribution
    clean_subject = adapt_content(subject)
    run(["git", "add", "-A"], cwd=str(our_repo))

    # Check if anything actually changed
    diff = run(["git", "diff", "--cached", "--name-only"], cwd=str(our_repo))
    if not diff.strip():
        print(f"    (no net changes after adaptation)")
        return False

    run(["git", "commit", "-m", clean_subject], cwd=str(our_repo))
    return True


def main() -> int:
    parser = argparse.ArgumentParser(description="Sync from HOC with adaptation")
    parser.add_argument("--dry-run", action="store_true",
                        help="Show what would happen without changes")
    parser.add_argument("--since", type=int, default=21,
                        help="Days back to sync (default: 21)")
    parser.add_argument("--our-repo", default=os.getcwd(),
                        help="Path to our repo (default: cwd)")
    args = parser.parse_args()

    our_repo = Path(args.our_repo).resolve()
    if not (our_repo / ".git").exists():
        print(f"[!] {our_repo} is not a git repo")
        return 1

    with tempfile.TemporaryDirectory(prefix="hoc_sync_") as tmp:
        upstream = Path(tmp) / "upstream"
        clone_upstream(upstream)

        commits = get_recent_commits(upstream, args.since)
        print(f"[*] Found {len(commits)} commits in the last {args.since} days")

        applied = 0
        skipped = 0
        for c in commits:
            if apply_commit(our_repo, upstream, c, args.dry_run):
                applied += 1
            else:
                skipped += 1

        print()
        print(f"[*] Applied:  {applied}")
        print(f"[*] Skipped:  {skipped}")
        if args.dry_run:
            print("[*] Dry run - no actual changes were made")

    return 0


if __name__ == "__main__":
    sys.exit(main())
