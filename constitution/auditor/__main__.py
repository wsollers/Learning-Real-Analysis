"""
__main__.py
Entry point for `python -m auditor`.
"""

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from auditor.cli import main

if __name__ == "__main__":
    main()
