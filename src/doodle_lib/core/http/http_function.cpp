//
// Created by TD on 2024/2/21.
//

#include "http_function.h"

#include <doodle_core/lib_warp/boost_fmt_error.h>
#include <doodle_core/logger/logger.h>

#include <doodle_lib/core/http/http_session_data.h>
#include <doodle_lib/core/http/http_websocket_data.h>
namespace doodle::http {
std::tuple<bool, http_function::capture_t> http_function::set_match_url(boost::urls::segments_ref in_segments_ref
) const {
  std::map<std::string, std::string> l_str{};
  if (in_segments_ref.size() != capture_vector_.size()) {
    return {false, {}};
  }
  for (const auto& [l_cap, l_seg] : ranges::zip_view(capture_vector_, in_segments_ref)) {
    if (!l_cap.is_capture && l_cap.name != l_seg) {
      return {false, {}};
    }
    l_str.emplace(l_cap.name, l_seg);
  }
  return {true, capture_t{l_str}};
}

}  // namespace doodle::http
