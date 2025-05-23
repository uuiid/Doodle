//
// Created by TD on 24-9-26.
//

#include "kitsu_front_end.h"

#include <doodle_lib/core/http/http_session_data.h>
namespace doodle::kitsu {
std::tuple<bool, http::capture_t> kitsu_front_end::set_match_url(boost::urls::segments_ref in_segments_ref) const {
  return {true, http::capture_t{}};
}

std::tuple<bool, http::capture_t> kitsu_proxy_url::set_match_url(boost::urls::segments_ref in_segments_ref) const {
  auto l_size = std::distance(in_segments_ref.begin(), in_segments_ref.end());
  if (l_size == 0) return {false, http::capture_t{}};

  bool l_result = true;

  std::int32_t l_index{0};
  for (auto&& i : in_segments_ref) {
    if (l_index == url_segments_.size()) break;
    l_result &= url_segments_[l_index] == i;
    ++l_index;
  }
  return {l_result, http::capture_t{}};
}
bool kitsu_proxy_url::is_proxy() const { return true; }
boost::asio::awaitable<boost::beast::http::message_generator> kitsu_proxy_url::callback(
    http::session_data_ptr in_handle
) {
  co_return in_handle->make_msg(nlohmann::json{});
}

}  // namespace doodle::kitsu