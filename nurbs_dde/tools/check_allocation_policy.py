#!/usr/bin/env python3
"""Fail if raw dynamic allocation escapes the allocator layer.

This is intentionally narrow for now: it blocks textual `new`, `delete`,
`malloc`, `calloc`, `realloc`, and `free` in project source files outside the
central memory package. Higher-level ownership cleanup can move smart-pointer
factories onto arena-aware builders without fighting this first guard.
"""

from __future__ import annotations

import re
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SOURCE_ROOTS = [ROOT / "src", ROOT / "tests"]
ALLOW_DIRS = {
    (ROOT / "src" / "memory").resolve(),
}
PATTERN = re.compile(r"\b(new|delete|malloc|calloc|realloc|free)\b")
EXTS = {".cpp", ".hpp", ".h", ".cc", ".cxx"}


def allowed(path: Path) -> bool:
    resolved = path.resolve()
    return any(allowed_dir in resolved.parents or resolved == allowed_dir for allowed_dir in ALLOW_DIRS)


def strip_line_comment(line: str) -> str:
    code = line.split("//", 1)[0]
    return code.replace("= delete", "")


def main() -> int:
    violations: list[str] = []
    for root in SOURCE_ROOTS:
        for path in root.rglob("*"):
            if path.suffix not in EXTS or allowed(path):
                continue
            for lineno, line in enumerate(path.read_text(encoding="utf-8", errors="ignore").splitlines(), 1):
                code = strip_line_comment(line)
                if PATTERN.search(code):
                    rel = path.relative_to(ROOT)
                    violations.append(f"{rel}:{lineno}: {line.strip()}")

    if violations:
        print("Raw allocation policy violation. Use the central allocation facility:")
        print("\n".join(violations))
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
