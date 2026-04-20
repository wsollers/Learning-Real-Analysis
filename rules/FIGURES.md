# Figure Rules

Use this module for TikZ and dependency-figure audits.

Core constraints:

- `figure-*.tex` files contain TikZ code only;
- figure files have no local preamble and no inline color definitions;
- dependency figures use standardized node colors and edge colors;
- dependency figures include a compact legend;
- dependency figures have explanatory captions and stable labels;
- dependency figures show structural backbone, not every minor remark.

Machine-readable rule objects live in `design-rules.json` with
`module: "figures"`.
