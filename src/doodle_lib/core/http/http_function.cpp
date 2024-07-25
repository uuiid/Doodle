//
// Created by TD on 2024/2/21.
//

#include "http_function.h"

#include <doodle_core/lib_warp/boost_fmt_error.h>
#include <doodle_core/logger/logger.h>

#include <doodle_lib/core/http/http_session_data.h>
namespace doodle::http {

std::vector<http_function::capture_data_t> http_function::set_cap_bit(std::string& in_str) {
  std::vector<std::string> l_vector{};
  boost::split(l_vector, in_str, boost::is_any_of("/"));
  std::erase_if(l_vector, [](const std::string& in_str) { return in_str.empty(); });

  std::vector<capture_data_t> l_capture_vector{l_vector.size()};
  for (size_t i = 0; i < l_vector.size(); ++i) {
    if (l_vector[i].front() == '{' && l_vector[i].back() == '}') {
      l_capture_vector[i].name       = l_vector[i].substr(1, l_vector[i].size() - 2);
      l_capture_vector[i].is_capture = true;
    } else {
      l_capture_vector[i].name       = l_vector[i];
      l_capture_vector[i].is_capture = false;
    }
  }
  return l_capture_vector;
}

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
    if (l_cap.is_capture && !l_seg.empty())
      l_str.emplace(l_cap.name, l_seg);
    else if (l_seg.empty())
      return {false, {}};
  }
  return {true, capture_t{l_str}};
}

}  // namespace doodle::http
