# =============================================================
# Dockerfile — LaTeX build environment
# Project: Learning Real Analysis
# Base:     texlive/texlive pinned by digest (full TeX Live, Debian)
#
# Engine:   LuaLaTeX (required for expl3, tcolorbox, TikZ stack)
# Driver:   latexmk  (multi-pass: aux, bib, index, toc, hyperref)
# =============================================================

FROM texlive/texlive:latest@sha256:2f9918a156cfc6c7527590e06f69bfb4fbc827278e4736c90e567382855d7cd1

# -------------------------------------------------------------
# System dependencies
#   latexmk     — multi-pass orchestration (already in texlive
#                 but we pin via apt for reliability)
#   biber       — BibLaTeX backend (not used yet but costs nothing)
#   perl        — latexmk runtime (present in base, made explicit)
#   fontconfig  — needed by LuaLaTeX for system font discovery
# -------------------------------------------------------------
RUN apt-get update && apt-get install -y --no-install-recommends \
        latexmk \
        biber \
        perl \
        fontconfig \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# -------------------------------------------------------------
# Rebuild the font cache so LuaLaTeX can find system fonts
# -------------------------------------------------------------
RUN fc-cache -fv

# -------------------------------------------------------------
# Working directory — the repo root is mounted here at runtime
# -------------------------------------------------------------
WORKDIR /workspace

# Default command: compile main.tex with LuaLaTeX via latexmk.
# The -interaction=nonstopmode flag prevents the compiler from
# halting on warnings, which would hang the container.
# Entrypoint is sh so we can run the full latexmk pipeline.
# -------------------------------------------------------------
CMD ["latexmk", \
     "-lualatex", \
     "-interaction=nonstopmode", \
     "-file-line-error", \
     "-synctex=1", \
     "-shell-escape", \
     "main.tex"]
