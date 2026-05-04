#pragma once
// memory/Containers.hpp
// Central container policy aliases.
//
// The architectural rule is that code depends on lifetime policy names, not
// directly on STL container choices.  FrameVector is already PMR-backed so
// frame-local storage can bind to MemoryService::frame(); the longer-lived
// aliases remain std-backed until their backing lifetimes are migrated.

#include <memory_resource>
#include <vector>

namespace ndde::memory {

template <class T>
using FrameVector = std::pmr::vector<T>;

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
