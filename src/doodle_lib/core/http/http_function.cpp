//
// Created by TD on 2024/2/21.
//

#include "http_function.h"

#include <doodle_core/lib_warp/boost_fmt_error.h>
#include <doodle_core/logger/logger.h>

#include <doodle_lib/core/http/http_session_data.h>
namespace doodle::http {
bool url_route_t::component_base_t::match(const std::string& in_str) const { return std::regex_match(in_str, regex_); }
void url_route_t::component_base_t::set(const std::string& in_str, const std::shared_ptr<void>& in_obj) const {}
uuid url_route_t::component_base_t::convert_uuid(const std::string& in_str) const {
  std::smatch l_result{};
  if (std::regex_match(in_str, l_result, regex_)) {
    for (auto& l_item : l_result) {
      return from_uuid_str(l_item.str());
    }
  }
  return {};
}
chrono::year_month url_route_t::component_base_t::convert_year_month(const std::string& in_str) const {
  std::smatch l_result{};
  chrono::year_month l_date{};
  if (std::regex_match(in_str, l_result, regex_)) {
    for (auto& l_item : l_result) {
      std::istringstream l_stream{l_item.str()};
      l_stream >> chrono::parse("%Y-%m", l_date);
      return l_date;
    }
  }
  return l_date;
}
chrono::year_month_day url_route_t::component_base_t::convert_year_month_day(const std::string& in_str) const {
  std::smatch l_result{};
  chrono::year_month_day l_date{};
  if (std::regex_match(in_str, l_result, regex_)) {
    for (auto& l_item : l_result) {
      std::istringstream l_stream{l_item.str()};
      l_stream >> chrono::parse("%Y-%m-%d", l_date);
      return l_date;
    }
  }
  return l_date;
}

void http_function_base_t::websocket_init(session_data_ptr in_handle) {}
boost::asio::awaitable<void> http_function_base_t::websocket_callback(
    boost::beast::websocket::stream<tcp_stream_type> in_stream, session_data_ptr in_handle
) {
  co_return;
}
bool http_function_base_t::has_websocket() const { return false; }
bool http_function_base_t::is_proxy() const { return false; }

std::vector<http_function::capture_data_t> http_function::set_cap_bit(std::string& in_str) {
  std::vector<std::string> l_vector{};
  boost::split(l_vector, in_str, boost::is_any_of("/"));
  std::erase_if(l_vector, [](const std::string& in_str) { return in_str.empty(); });

  std::vector<capture_data_t> l_capture_vector{l_vector.size()};
  for (size_t i = 0; i < l_vector.size(); ++i) {
    if (!l_vector[i].empty() && l_vector[i].front() == '{' && l_vector[i].back() == '}') {
      l_capture_vector[i].name       = l_vector[i].substr(1, l_vector[i].size() - 2);
      l_capture_vector[i].is_capture = true;
    } else {
      l_capture_vector[i].name       = l_vector[i];
      l_capture_vector[i].is_capture = false;
    }
  }
  return l_capture_vector;
}

std::tuple<bool, http_function::capture_t> http_function::set_match_url(
    boost::urls::segments_ref in_segments_ref
) const {
  std::map<std::string, std::string> l_str{};

  std::vector<std::string> l_segments_ref_not_null = in_segments_ref |
                                                     ranges::views::filter([](auto&& i) { return !i.empty(); }) |
                                                     ranges::to<std::vector<std::string>>;

  if (l_segments_ref_not_null.size() != capture_vector_.size()) {
    return {false, {}};
  }
  for (const auto& [l_cap, l_seg] : ranges::zip_view(capture_vector_, l_segments_ref_not_null)) {
    if (!l_cap.is_capture && l_cap.name != l_seg) {
      return {false, {}};
    }
    if (l_cap.is_capture && !l_seg.empty()) l_str.emplace(l_cap.name, l_seg);
    // else if (l_seg.empty())
    //   return {false, {}};
  }
  return {true, capture_t{l_str}};
}

}  // namespace doodle::http
