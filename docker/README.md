# Docker — LaTeX Build Environment

This folder contains the reproducible LaTeX build environment for the
**Learning Real Analysis** project. Anyone with Docker installed can
compile the notes to PDF without installing a local TeX distribution.

---

## Quick Start

```powershell
# 1. Build the Docker image (once — ~4 GB, full TeX Live)
.\docker\compile.ps1 -Build

# 2. Compile the full book
.\docker\compile.ps1

# 3. Compile a single volume
.\docker\compile.ps1 -Volume iii

# 4. Open the PDF immediately after a successful build
.\docker\compile.ps1 -Open
.\docker\compile.ps1 -Volume iii -Open
```

Full book output: `build\main.pdf`
Volume output:    `build\volume-N-main.pdf`

---

## Files

| File | Purpose |
|---|---|
| `Dockerfile` | Defines the build image (full TeX Live + latexmk) |
| `compile.ps1` | PowerShell convenience script — wraps all Docker commands |
| `../.latexmkrc` | latexmk config at repo root — engine, indexes, output dirs |

---

## Volume Roots

Each volume has a thin standalone root at the repo root:

| File | Content |
|---|---|
| `main.tex` | Full book — all five volumes |
| `volume-i-main.tex` | Volume I: Mathematical Logic |
| `volume-ii-main.tex` | Volume II: Foundations of Formal Number Systems |
| `volume-iii-main.tex` | Volume III: Abstract Mathematics |
| `volume-iv-main.tex` | Volume IV: Applied and Computational Mathematics |
| `volume-v-main.tex` | Volume V: Numerical Analysis and Approximation |

All thin roots share `common/volume-preamble.tex` and delegate entirely
to their `volume-N/index.tex` — no content files are duplicated.

**On Overleaf:** Menu → Main Document → select whichever root you want.

---

## Engine & Pipeline

| Layer | Choice | Reason |
|---|---|---|
| Engine | **LuaLaTeX** | Required by `expl3`, `tcolorbox` skins, `microtype` |
| Driver | **latexmk** | Multi-pass orchestration: aux → bib → index → toc → hyperref |
| Bibliography | **BibTeX / natbib** | `plainnat` style via `bibliography/analysis.bib` |
| Indexes | **makeindex** (×2) | `imakeidx` produces `main.tech.idx` and `main.dep.idx` |
| Output dir | `build/` | Keeps the repo root clean |

---

## Script Options

```powershell
.\docker\compile.ps1                           # Full book (main.tex)
.\docker\compile.ps1 -Volume i                 # Volume I only
.\docker\compile.ps1 -Volume ii                # Volume II only
.\docker\compile.ps1 -Volume iii               # Volume III only
.\docker\compile.ps1 -Volume iv                # Volume IV only
.\docker\compile.ps1 -Volume v                 # Volume V only
.\docker\compile.ps1 -Build                    # Rebuild the Docker image first
.\docker\compile.ps1 -Clean                    # latexmk -C (wipe build/) then compile
.\docker\compile.ps1 -Open                     # Open PDF in default viewer after build
.\docker\compile.ps1 -Volume iii -Clean -Open  # All combined
```

---

## First-Time Setup

Docker Desktop must be installed and running with the WSL2 backend enabled.
The image only needs to be built once:

```powershell
.\docker\compile.ps1 -Build
```

Subsequent `compile.ps1` calls reuse the cached image and are fast.

---

## Updating the Image

If you add packages to `common/preamble.tex` that require a newer TeX Live,
rebuild the image:

```powershell
.\docker\compile.ps1 -Build
```

The `texlive/texlive:latest` base image is full TeX Live — virtually all
CTAN packages are already present and no `tlmgr install` calls are needed.

---

## Troubleshooting

| Symptom | Fix |
|---|---|
| `Image not found` error | Run `.\docker\compile.ps1 -Build` |
| Build fails with LaTeX errors | Check `build\main.log` for the full compiler log |
| Index not updating | Run `.\docker\compile.ps1 -Clean` to wipe stale `.idx` files |
| PDF not found after success | Verify `build\` directory exists; check `.latexmkrc` `$out_dir` |
| Docker not running | Start Docker Desktop from the system tray |
| TOC missing | Run `.\docker\compile.ps1 -Clean` to force all passes from scratch |
