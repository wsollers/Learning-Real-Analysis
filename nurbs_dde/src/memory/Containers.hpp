#pragma once
// memory/Containers.hpp
// Central container policy aliases.
//
// These aliases intentionally start as std-backed containers. The architectural
// rule is that code depends on lifetime policy names, not directly on STL
// container choices. That lets the memory package later swap these aliases to
// arena/PMR-backed implementations without touching render or simulation call
// sites.

#include <vector>

namespace ndde::memory {

template <class T>
using FrameVector = std::vector<T>;

template <class T>
using ViewVector = std::vector<T>;

template <class T>
using SimVector = std::vector<T>;

template <class T>
using CacheVector = std::vector<T>;

template <class T>
using HistoryVector = std::vector<T>;

template <class T>
using PersistentVector = std::vector<T>;

} // namespace ndde::memory
