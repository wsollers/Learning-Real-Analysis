# lra-nurbs Overlay

Stub overlay for C++ / Vulkan / geometry / simulation work.

Owned concerns:

- C++ and CMake build rules,
- Vulkan rendering rules,
- geometry and NURBS implementation,
- simulation and DDE implementation,
- local validators and CI expectations.

## Agent Scope

C++ / Vulkan / geometry / simulation rules apply only to `lra-nurbs` and the
monorepo `nurbs_dde/` mirror. They must not be injected into volume content
instructions.

Use local CMake, tests, and repo validators for implementation changes.
