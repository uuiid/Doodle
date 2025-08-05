//
// Created by TD on 24-9-26.
//

#pragma once
#include <doodle_lib/core/http/http_function.h>

#include <cache.hpp>
#include <cache_policy.hpp>
#include <lru_cache_policy.hpp>
namespace doodle::http {
DOODLE_HTTP_FUN(kitsu_front_end)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_OVERRIDE(head)
using cache_type = caches::fixed_sized_cache<FSys::path, std::string, caches::LRUCachePolicy>;
std::shared_ptr<FSys::path> root_path_{};
std::shared_ptr<cache_type> cache_;
explicit kitsu_front_end(const FSys::path& in_root)
    : root_path_(std::make_shared<FSys::path>(in_root)), cache_(std::make_shared<cache_type>(1024)) {}
DOODLE_HTTP_FUN_END()

class kitsu_front_end_url_route_component : public url_route_component_base_t {
 public:
  std::tuple<bool, std::shared_ptr<http_function>> set_match_url(
      boost::urls::segments_ref in_segments_ref, const std::shared_ptr<http_function>& in_data
  ) const override;
};

}  // namespace doodle::http