# lra-numerical-analysis Overlay

Overlay for the numerical-analysis software workbench.

Owned concerns:

- numerical methods,
- computational C++ experiments,
- project-local tests, benchmarks, and fuzzing,
- plotting and Python analysis,
- LaTeX-ready numerical reports.

## Agent Scope

Numerical-analysis guidance applies only to `lra-numerical-analysis`. It may
cover numerical C++ experiments, benchmarking, plotting, and report generation,
but it must not be injected into volume content instructions.

This repo does not own Lean formalization, NURBS/Vulkan simulation, or shared
LaTeX infrastructure.

## Workspace Shape

The repo is a C++23 numerical software laboratory. Prefer this layout:

```text
include/lra/numeric/        shared numerical library headers
src/numeric/                shared numerical library implementations
projects/<name>/            self-contained lab projects
analysis/                   Python analysis scripts and notes
docker/                     portable Clang and analysis images
tools/                      local build helpers
artifacts/                  generated data, plots, and reports
```

Each `projects/<name>/` should be self-contained. Project-local tests,
benchmarks, and fuzz targets belong under that project:

```text
projects/<name>/
  CMakeLists.txt
  src/
  tests/
  benchmarks/
  fuzz/
  README.md
```

Shared reusable numerical code belongs in `include/lra/numeric/` and
`src/numeric/`. Do not hide project-specific experiments in the shared library
until at least two projects need the abstraction.

## Dependency Boundaries

Core numerical code must not depend on Vulkan, ImGui, GLFW, or UI code. Visual
lab projects may depend on Vulkan, ImGui, GLFW, shaders, and assets, but those
dependencies must stay below the project/application boundary.

Tests, benchmarks, fuzz targets, and Python analysis should be able to exercise
the shared numerical core without launching a graphical application.

## Validation Expectations

New numerical algorithms require tests. Benchmark and fuzz coverage scale with
risk:

- floating-point representation, rounding, interval arithmetic, and error-bound
  code should usually have unit tests plus focused fuzz/property tests;
- performance-sensitive algorithms should have project-local Google Benchmark
  targets;
- visualization projects should keep numerical correctness tests separate from
  Vulkan/ImGui smoke tests.

The standard local gates are:

- MSVC x64 build from a Visual Studio developer environment;
- Dockerized Clang/Ninja build;
- project-local GoogleTest/CTest success;
- benchmark and fuzz gates when the touched project owns them.

## Artifact And Analysis Flow

C++ projects should write structured artifacts such as JSON or CSV under
`artifacts/<project>/`. Python analysis consumes those artifacts and produces
plots, tables, and reports.

The analysis Docker image is the portable environment for NumPy, SciPy, pandas,
matplotlib, seaborn, notebooks, and report-generation scripts. Ordinary C++
unit-test builds must not require the Python analysis image.

Small curated plots may be committed when they are curriculum inputs. Large
benchmark outputs, generated scratch data, and transient fuzz corpora should
remain untracked unless a task explicitly promotes them to source material.
