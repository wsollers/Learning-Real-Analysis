#!/usr/bin/env python3
"""Select scoped DESIGN rule objects for prompts or tools."""

from __future__ import annotations

import argparse
import json
from pathlib import Path


def repo_root(start: Path) -> Path:
    current = start.resolve()
    if current.is_file():
        current = current.parent
    for path in [current, *current.parents]:
        if (path / "rules" / "design-rules.json").exists():
            return path
    raise SystemExit("Could not find rules/design-rules.json")


def main() -> int:
    parser = argparse.ArgumentParser(description="Select DESIGN.md rules by module, workflow tag, or status.")
    parser.add_argument("--module", action="append", default=[], help="Rule module, e.g. definitions, notation, proofs.")
    parser.add_argument("--applies-to", action="append", default=[], help="Workflow tag, e.g. definition_generation.")
    parser.add_argument("--status", action="append", default=[], help="Implementation status, e.g. planned.")
    parser.add_argument("--ids-only", action="store_true", help="Print only matching rule IDs.")
    parser.add_argument("--registry", default=None, help="Optional path to design-rules.json.")
    args = parser.parse_args()

    root = repo_root(Path.cwd())
    registry = Path(args.registry) if args.registry else root / "rules" / "design-rules.json"
    data = json.loads(registry.read_text(encoding="utf-8"))
    rules = data["rules"]

    def keep(rule: dict) -> bool:
        if args.module and rule.get("module") not in set(args.module):
            return False
        if args.status and rule.get("implementation_status") not in set(args.status):
            return False
        if args.applies_to:
            tags = set(rule.get("applies_to", []))
            if not tags.intersection(args.applies_to):
                return False
        return True

    selected = [rule for rule in rules if keep(rule)]
    if args.ids_only:
        for rule in selected:
            print(rule["id"])
    else:
        print(json.dumps({"count": len(selected), "rules": selected}, indent=2, ensure_ascii=False))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
