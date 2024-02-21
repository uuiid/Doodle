//
// Created by TD on 2024/2/21.
//

#include "http_route.h"
namespace doodle::http {
void http_route::capture_url::set_cap_bit() {
  for (const auto& l_str : capture_vector_) {
    if (l_str.front() == '{' && l_str.back() == '}') {
      capture_bitset_.push_back(true);
    } else {
      capture_bitset_.push_back(false);
    }
  }
}
std::tuple<bool, std::map<std::string, std::string>> http_route::capture_url::match_url(
    boost::urls::segments_ref in_segments_ref
) const {
  auto l_it = in_segments_ref.begin();
  std::map<std::string, std::string> l_str{};
  if (in_segments_ref.size() != capture_vector_.size()) {
    return {false, l_str};
  }
  bool l_result{true};
  for (auto i = 0; l_it != in_segments_ref.end(); ++l_it, ++i) {
    if (capture_bitset_[i]) {
      l_str.emplace(capture_vector_[i].substr(1, capture_vector_[i].size() - 2), *l_it);
    } else {
      if (capture_vector_[i] != *l_it) {
        l_result = false;
        break;
      }
    }
  }
  return {l_result, l_str};
}
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