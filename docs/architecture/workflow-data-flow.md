# Workflow And Data Flow Architecture

This document maps the operational workflows across the LRA repository family.
It is a routing aid: ownership rules remain in the focused architecture and
governance documents linked from `docs/architecture/README.md`.

## Repository Workflow Map

```mermaid
flowchart LR
    governance["lra-governance<br/>standards, prompts, validators, wrappers"]
    common["lra-common<br/>shared LaTeX infrastructure"]
    volumes["lra-volume-i ... lra-volume-viii<br/>volume-owned content"]
    mono["Learning-Real-Analysis<br/>integrated book source"]
    lean["lra-lean<br/>formalization"]
    nurbs["lra-nurbs<br/>geometry and simulation"]
    numerical["lra-numerical-analysis<br/>benchmarks and numerical notes"]
    extractor["lra-pdf-extractor<br/>candidate source ingestion"]
    profiles["lra-source-profiles<br/>source profile staging"]
    explorer["lra-knowledge-explorer<br/>graph extraction and explorer"]
    output["lra-volumes-output<br/>published PDFs"]

    governance -->|"governance sync / wrapper generation"| common
    governance -->|"governance sync / wrapper generation"| volumes
    governance -->|"governance sync / wrapper generation"| mono
    governance -->|"governance sync / wrapper generation"| lean
    governance -->|"governance sync / wrapper generation"| nurbs
    governance -->|"governance sync / wrapper generation"| numerical
    governance -->|"governance sync / wrapper generation"| extractor
    governance -->|"governance sync / wrapper generation"| profiles
    governance -->|"governance sync / wrapper generation"| explorer

    common -->|"common/, bibliography/, macros, preambles"| volumes
    common -->|"common/, bibliography/, macros, preambles"| mono
    volumes -->|"volume content sync"| mono
    lean -->|"Lean source sync"| mono
    nurbs -->|"specialist source sync"| mono

    extractor -. reviewed candidate artifacts only .-> mono
    profiles -. reviewed candidate artifacts only .-> mono

    volumes -->|"Docker PDF workflow"| output
    mono -->|"Docker full-book PDF workflow"| output
    mono -->|"dispatch / integrated source input"| explorer
    explorer -->|"generated graph data and static explorer"| explorer
```

## Build, Publish, And Knowledge Data Flow

```mermaid
flowchart TB
    subgraph authoring["Authoring Sources"]
        volumeSource["Volume source repos<br/>lra-volume-*"]
        commonSource["Shared LaTeX source<br/>lra-common"]
        monoSource["Integrated source<br/>Learning-Real-Analysis"]
    end

    subgraph build["PDF Build Workflow"]
        dockerfile["docker/Dockerfile<br/>pinned TeX Live image"]
        image["learning-real-analysis-latex<br/>CI-built Docker image"]
        latexmk["latexmk + LuaLaTeX<br/>inside Docker"]
        verify["PDF validation<br/>nonempty, %PDF-, %%EOF"]
    end

    subgraph publish["Publication"]
        outputRepo["lra-volumes-output"]
        pagesOrLinks["download links / published PDFs"]
    end

    subgraph knowledge["Knowledge Explorer Pipeline"]
        extraction["theorem and dependency extraction"]
        graphData["knowledge.json<br/>graph-edges.json"]
        explorerApp["lra-knowledge-explorer"]
        pagesExplorer["GitHub Pages explorer"]
    end

    commonSource -->|"synced infrastructure"| volumeSource
    commonSource -->|"synced infrastructure"| monoSource
    volumeSource -->|"volume sync"| monoSource

    volumeSource --> dockerfile
    monoSource --> dockerfile
    dockerfile --> image
    image --> latexmk
    latexmk --> verify
    verify -->|"copy only after validation"| outputRepo
    outputRepo --> pagesOrLinks

    monoSource --> extraction
    extraction --> graphData
    graphData --> explorerApp
    explorerApp --> pagesExplorer
```

## Reading Rules

- Solid arrows are approved sync, build, or generation paths.
- Dotted arrows are staging paths that require review before content enters an
  owning source repository.
- PDF workflows must build through the checked-in Docker image definition and
  must validate the produced PDF before publishing to `lra-volumes-output`.
- Generated explorer data is derived from integrated source structure; it is
  not hand-authored in volume repositories.
