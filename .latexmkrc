# =============================================================
# .latexmkrc — Learning Real Analysis
# =============================================================

# Engine: LuaLaTeX
$pdf_mode = 4;
$lualatex = 'lualatex -interaction=nonstopmode -file-line-error -synctex=1 -shell-escape %O %S';

# Bibliography: natbib + BibTeX
$bibtex_use = 1;

# Let imakeidx / latexmk handle indexes naturally.
# Do NOT force makeindex before the .idx files exist.

# Pass count
$max_repeat = 8;

# Build directories
$out_dir = 'build';
$aux_dir = 'build';

# Viewer
$pdf_previewer = 'start %S';

# Cleanup
$clean_ext = 'aux bbl blg fdb_latexmk fls idx ilg ind lof lot out run.xml synctex.gz toc ' .
             'tech.idx tech.ind tech.ilg dep.idx dep.ind dep.ilg ' .
             'nav snm vrb xdv';