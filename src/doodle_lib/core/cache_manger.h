//
// Created by TD on 24-12-19.
//
#pragma once

#include <doodle_core/doodle_core_fwd.h>

#include <cache.hpp>
#include <cache_policy.hpp>
#include <lru_cache_policy.hpp>
#include <nlohmann/json.hpp>
namespace doodle {
class cache_manger {
  template <typename Key, typename Value>
  using lru_cache_t = typename caches::fixed_sized_cache<Key, Value, caches::LRUCachePolicy>;

  struct cache_value {
    nlohmann::json json_;
    std::chrono::time_point<std::chrono::system_clock> time_point_;
  };

  using cache_type = lru_cache_t<uuid, cache_value>;
  cache_type cache_;

 public:
  cache_manger() : cache_(1024 * 1024 * 1024) {}

  void set(const uuid& id, const nlohmann::json& json) {
    cache_.Put(id, cache_value{json, std::chrono::system_clock::now()});
  }

  std::optional<nlohmann::json> get(const uuid& id) {
    if (auto l_value = cache_.TryGet(id); l_value.second) {
      return l_value.first->json_;
    }
    return std::nullopt;
  }
  void erase(const uuid& id) { cache_.Remove(id); }
};
}  // namespace doodle