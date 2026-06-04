# Assembled Bibliography Shards

This directory is an integration target in `Learning-Real-Analysis`.

Each `volume-*.bib` file is owned by the corresponding `lra-volume-*` repository
and is copied here by that volume's sync workflow. Do not edit these files here
as the source of truth; edit the owning volume repo and sync the shard in.

The full monorepo build assembles the bibliography by listing all volume-owned
shards in `main.tex`. Per-volume monorepo roots list only their corresponding
volume shard.

Run duplicate checks with:

```powershell
python scripts/check_bibliography.py --bib-dir bibliography
```