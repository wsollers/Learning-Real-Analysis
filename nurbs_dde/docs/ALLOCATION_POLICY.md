# Allocation Policy

The current hard rule is enforced by `tools/check_allocation_policy.py`:

- raw `new`
- raw `delete`
- `malloc`
- `calloc`
- `realloc`
- `free`

These are allowed only inside the central memory package.

`std::vector`, `std::string`, and `std::unique_ptr` still exist in app/runtime code during the refactor. Direct `std::vector` use should keep shrinking behind the policy aliases, especially for simulation hot-path state, render packets, interaction state, and engine service registration/storage.

Recommended next enforcement stages:

1. Keep the raw allocation ban always on.
2. Use the policy aliases in `memory/Containers.hpp` for dynamic arrays:
   `FrameVector<T>` for per-frame scratch,
   `ViewVector<T>` for render/input view lifetime,
   `SimVector<T>` for simulation-instance lifetime,
   `CacheVector<T>` for derived surface/geometry caches,
   `HistoryVector<T>` for trails, delay buffers, replay/export history,
   and `PersistentVector<T>` for app/service/session lifetime.
3. Protect migrated hot-path files with `tools/check_hot_path_container_policy.py`.
4. Move the policy aliases from `std::vector` to arena/PMR-backed storage.
5. Require particle/runtime/history buffers to use those aliases.
6. Leave configuration and scalar/string metadata on ordinary STL until their lifetime pressure justifies migration.

Currently protected migrated areas include render packets, interaction/picking state,
view registration and mouse state, surface mesh caches, particle/trail/swarm state,
history buffers, simulation context command queues, engine panels/hotkeys, scoped
service handles, simulation runtime registry storage, and scene snapshots.
