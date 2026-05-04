# =============================================================
# .latexmkrc — latexmk configuration for Learning Real Analysis
# Place this file at the repo root (alongside main.tex)
#
# This config handles the full compilation pipeline:
#   LuaLaTeX  — primary engine (expl3, tcolorbox, TikZ, microtype)
#   natbib    — bibliography via bibtex (plainnat style)
#   imakeidx  — two named indexes: [tech] and [dep]
#   synctex   — enabled for editor forward/back-search
# =============================================================

# ── Engine ────────────────────────────────────────────────────
# Use LuaLaTeX for all PDF generation.
$pdf_mode     = 4;       # 4 = lualatex -> PDF directly
$lualatex     = 'lualatex -interaction=nonstopmode -file-line-error -synctex=1 -shell-escape %O %S';
$pdf_previewer = 'start %S';  # Windows: open PDF in default viewer

# ── Bibliography ──────────────────────────────────────────────
# natbib uses classic BibTeX (not biber).
$bibtex_use = 1;         # run bibtex when .bib files change

# ── Index generation ──────────────────────────────────────────
# imakeidx produces two named indexes:
#   main.tech.idx  →  Technique Index
#   main.dep.idx   →  Dependency Index
#
# latexmk's add_cus_dep wires up the .idx → .ind pipeline for
# each named index so they are rebuilt only when changed.
add_cus_dep('tech.idx', 'tech.ind', 0, 'run_makeindex_tech');
add_cus_dep('dep.idx',  'dep.ind',  0, 'run_makeindex_dep');

sub run_makeindex_tech {
    system("makeindex -s '$_[0].ist' -t '$_[0].tech.ilg' -o '$_[0].tech.ind' '$_[0].tech.idx'");
}

sub run_makeindex_dep {
    system("makeindex -s '$_[0].ist' -t '$_[0].dep.ilg' -o '$_[0].dep.ind' '$_[0].dep.idx'");
}

# ── Pass count ───────────────────────────────────────────────
# Default is 5; your document needs at least 4 passes on a clean
# build (toc write → bib → cleveref/hyperref resolve → final).
# Setting 8 gives headroom for the two named indexes as well.
$max_repeat = 8;

# Teach latexmk to recognise rerun signals from natbib and
# cleveref that it does not detect by default.
$lualatex .= ' | grep -qE "(Rerun|rerun|undefined references|Citation.s. may have changed)"';

# ── Output and aux directories ────────────────────────────────
# Keep the root clean. All intermediate files go to build/.
# The final PDF is written back to the repo root for convenience.
$out_dir      = 'build';
$aux_dir      = 'build';

# ── Cleanup extensions ────────────────────────────────────────
# Files removed by `latexmk -c` (clean) and `latexmk -C` (full clean)
$clean_ext    = 'aux bbl blg fdb_latexmk fls idx ilg ind lof lot out run.xml synctex.gz toc ' .
                'tech.idx tech.ind tech.ilg dep.idx dep.ind dep.ilg ' .
                'nav snm vrb xdv';
