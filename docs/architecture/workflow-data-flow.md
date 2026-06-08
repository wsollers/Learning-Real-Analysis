# Workflow And Data Flow Architecture

This document maps the operational workflows across the LRA repository family.
It is a routing aid: ownership rules remain in the focused architecture and
governance documents linked from `docs/architecture/README.md`.

## Repository Workflow Map

```mermaid
flowchart LR
    governance["lra-governance<br/>canonical standards, prompts, validators"]
    wrapper["leaf governance wrappers<br/>delegate at runtime"]
    missing["clear failure<br/>lra-governance not present"]
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

    governance -->|"sync governance docs and thin wrappers"| common
    governance -->|"sync governance docs and thin wrappers"| volumes
    governance -->|"sync governance docs and thin wrappers"| mono
    governance -->|"sync governance docs and thin wrappers"| lean
    governance -->|"sync governance docs and thin wrappers"| nurbs
    governance -->|"sync governance docs and thin wrappers"| numerical
    governance -->|"sync governance docs and thin wrappers"| extractor
    governance -->|"sync governance docs and thin wrappers"| profiles
    governance -->|"sync governance docs and thin wrappers"| explorer

    common --> wrapper
    volumes --> wrapper
    mono --> wrapper
    lean --> wrapper
    nurbs --> wrapper
    numerical --> wrapper
    extractor --> wrapper
    profiles --> wrapper
    explorer --> wrapper
    wrapper -->|"find sibling repo or LRA_GOVERNANCE_ROOT"| governance
    wrapper -. abort if unavailable .-> missing

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
        governanceSource["Governance source<br/>lra-governance"]
    end

    subgraph governanceRuntime["Governance Checks"]
        leafWrappers["Leaf repo wrappers<br/>tools/governance and scripts"]
        canonicalChecks["Canonical validators<br/>lra-governance/tools/governance"]
        governanceMissing["Fail with actionable message<br/>lra-governance is not present"]
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
    governanceSource -->|"synced guidance and wrappers"| volumeSource
    governanceSource -->|"synced guidance and wrappers"| commonSource
    governanceSource -->|"synced guidance and wrappers"| monoSource
    volumeSource -->|"volume sync"| monoSource
    volumeSource --> leafWrappers
    commonSource --> leafWrappers
    monoSource --> leafWrappers
    leafWrappers -->|"delegate"| canonicalChecks
    canonicalChecks -->|"governance pass/fail"| volumeSource
    leafWrappers -. governance root missing .-> governanceMissing

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
- Leaf repository governance tools are wrappers only. They delegate to the
  canonical implementations in `lra-governance/tools/governance`, using a
  sibling `lra-governance` checkout or `LRA_GOVERNANCE_ROOT`; if neither is
  available, they fail with a clear setup message.
- PDF workflows must build through the checked-in Docker image definition and
  must validate the produced PDF before publishing to `lra-volumes-output`.
- Generated explorer data is derived from integrated source structure; it is
  not hand-authored in volume repositories.
