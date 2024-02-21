//
// Created by TD on 2024/2/21.
//

#include "http_route.h"
namespace doodle::http {

http_route::action_type http_route::capture_url::operator()(boost::urls::segments_ref in_segments_ref) const {
  auto [l_result, l_map] = match_url(in_segments_ref);
  if (l_result) {
    return [map_ = l_map, call = action_](const entt::handle& in_handle) {
      in_handle.emplace_or_replace<render_farm::session::capture_url>(map_);
      call(in_handle);
    };
  } else
    return {};
}
}  // namespace doodle::http