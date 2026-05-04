#pragma once
// memory/Containers.hpp
// Central container policy aliases.
//
// These aliases intentionally start as std-backed containers. The architectural
// rule is that hot-path code depends on the policy names, not directly on STL
// container choices. That lets the memory package later swap FrameVector and
// PersistentVector to arena/PMR-backed implementations without touching render
// or simulation call sites.

#include <vector>

namespace ndde::memory {

template <class T>
using FrameVector = std::vector<T>;

template <class T>
using PersistentVector = std::vector<T>;

template <class T>
using SimVector = std::vector<T>;

} // namespace ndde::memory
