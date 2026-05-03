# Allocation Policy

The current hard rule is enforced by `tools/check_allocation_policy.py`:

- raw `new`
- raw `delete`
- `malloc`
- `calloc`
- `realloc`
- `free`

These are allowed only inside the central memory package.

`std::vector`, `std::string`, and `std::unique_ptr` still exist in app/runtime code during the refactor. They are acceptable for low-frequency ownership and UI/service registration code for now, but simulation hot-path state should migrate toward arena-backed storage.

Recommended next enforcement stages:

1. Keep the raw allocation ban always on.
2. Introduce arena-backed aliases for simulation-owned dynamic arrays.
3. Require particle/runtime/history buffers to use those aliases.
4. Leave configuration, metadata, and panel registration on ordinary STL until their lifetime pressure justifies migration.
5. Add a second static check that flags direct `std::vector` use under selected hot-path folders once the aliases are ready.
