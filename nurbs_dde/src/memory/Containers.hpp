#pragma once
// memory/Containers.hpp
// Central container policy aliases.
//
// The architectural rule is that code depends on lifetime policy names, not
// directly on STL container choices.  All lifetime aliases are PMR-backed so
// storage can bind to the matching MemoryService scope.

#include <memory_resource>
#include <vector>

namespace ndde::memory {

template <class T>
using FrameVector = std::pmr::vector<T>;

template <class T>
using ViewVector = std::pmr::vector<T>;

template <class T>
using SimVector = std::pmr::vector<T>;

template <class T>
using CacheVector = std::pmr::vector<T>;

template <class T>
using HistoryVector = std::pmr::vector<T>;

template <class T>
using PersistentVector = std::pmr::vector<T>;

} // namespace ndde::memory
