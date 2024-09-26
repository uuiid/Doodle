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
  return {false, http::capture_t{}};
}
}  // namespace doodle::kitsu