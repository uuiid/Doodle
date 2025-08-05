//
// Created by TD on 24-9-26.
//

#pragma once
#include <doodle_lib/core/http/http_function.h>

namespace doodle::http {
DOODLE_HTTP_FUN(kitsu_front_end)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_OVERRIDE(head)
std::shared_ptr<FSys::path> root_path_{};
explicit kitsu_front_end(const FSys::path& in_root) : root_path_(std::make_shared<FSys::path>(in_root)) {}
DOODLE_HTTP_FUN_END()

class kitsu_front_end_url_route_component : public url_route_component_base_t {
 public:
  std::tuple<bool, std::shared_ptr<http_function>> set_match_url(
      boost::urls::segments_ref in_segments_ref, const std::shared_ptr<http_function>& in_data
  ) const override;
};

}  // namespace doodle::http