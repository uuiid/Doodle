//
// Created by TD on 24-9-26.
//

#include "kitsu_front_end.h"

#include <doodle_lib/core/http/http_session_data.h>
namespace doodle::kitsu {
std::tuple<bool, http::capture_t> kitsu_front_end::set_match_url(boost::urls::segments_ref in_segments_ref) const {
  auto l_path = *root_path_;
  for (auto&& i : in_segments_ref) {
    l_path /= i;
  }
  if (FSys::exists(l_path)) {
    return {true, http::capture_t{}};
  }
  return {true, http::capture_t{}};
}

std::tuple<bool, http::capture_t> kitsu_proxy_url::set_match_url(boost::urls::segments_ref in_segments_ref) const {
  if (url_segments_.empty()) return {false, http::capture_t{}};

  bool l_result = true;

  std::int32_t l_index{0};
  for (auto&& i : in_segments_ref) {
    if (l_index == url_segments_.size()) break;
    l_result &= url_segments_[l_index] == i;
    ++l_index;
  }
  return {l_result, http::capture_t{}};
}

}  // namespace doodle::kitsu