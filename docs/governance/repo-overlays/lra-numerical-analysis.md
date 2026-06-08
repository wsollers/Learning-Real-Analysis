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

## Unit-Test Project Workflow

When creating a unit-test-only project, follow the existing `hello-unit`
pattern unless local files clearly establish a newer convention:

1. Create `projects/<name>/`.
2. Create `projects/<name>/tests/`.
3. Add `projects/<name>/CMakeLists.txt` with `add_subdirectory(tests)`.
4. Add `projects/<name>/tests/CMakeLists.txt` defining one GoogleTest
   executable target for the project.
5. Name the initial test source `<name>-test.cpp` unless the repository's
   nearby examples use a more specific `test_<name>.cpp` convention.
6. Link the test target to `lra::numeric` and `GTest::gtest_main`.
7. Apply the standard local target helpers, including `lra_configure_target`
   and `lra_enable_sanitizers`, when they are available.
8. Register tests with `gtest_discover_tests`.
9. Add `add_subdirectory(projects/<name>)` to the root `CMakeLists.txt`.
10. Start with a compile-safe smoke test that asserts `true`.
11. Build the project and run CTest before reporting completion.

If the project introduces reusable numerical declarations, place public headers
under `include/lra/numeric/`. Keep project-specific test fixtures and helper
code under `projects/<name>/tests/` until at least two projects need the shared
abstraction.

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

On Windows, run MSVC configure, build, and test commands from a Visual Studio
Developer Command Prompt, or through the repo's `tools/build-msvc.ps1` helper
that initializes `vcvars64.bat`. A plain PowerShell session may find stale or
missing compiler paths and is not a valid substitute for the MSVC gate.

## Pre-Push Gates

Before pushing numerical-analysis changes, both platform gates must be green:

```powershell
.\tools\build-msvc.ps1
.\tools\build-docker-clang.ps1
```

The MSVC gate validates the Windows Visual Studio toolchain. The Docker gate
validates the Linux Clang/Ninja toolchain and runs CTest inside the container.

If Docker commands fail with a Docker engine pipe error such as
`open //./pipe/dockerDesktopLinuxEngine`, start Docker Desktop, wait for the
engine to become available, and rerun the Docker gate. Docker Desktop is not
assumed to start automatically on Windows workstations.

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
