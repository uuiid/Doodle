//
// Created by TD on 2024/2/21.
//

#include "http_function.h"

#include <doodle_core/lib_warp/boost_fmt_error.h>
#include <doodle_core/logger/logger.h>

#include <doodle_lib/core/http/http_session_data.h>
namespace doodle::http {
bool url_route_component_t::component_base_t::match(const std::string& in_str) const {
  return std::regex_match(in_str, regex_);
}
bool url_route_component_t::component_base_t::set(
    const std::string& in_str, const std::shared_ptr<void>& in_obj
) const {
  return std::regex_match(in_str, regex_);
}
std::tuple<bool, uuid> url_route_component_t::component_base_t::convert_uuid(const std::string& in_str) const {
  std::smatch l_result{};
  if (std::regex_match(in_str, l_result, regex_)) {
    for (auto& l_item : l_result) {
      return {true, from_uuid_str(l_item.str())};
    }
  }
  return {false, uuid{}};
}
std::tuple<bool, chrono::year_month> url_route_component_t::component_base_t::convert_year_month(
    const std::string& in_str
) const {
  std::smatch l_result{};
  chrono::year_month l_date{};
  if (std::regex_match(in_str, l_result, regex_)) {
    for (auto& l_item : l_result) {
      std::istringstream l_stream{l_item.str()};
      l_stream >> chrono::parse("%Y-%m", l_date);
      return {true, l_date};
    }
  }
  return {false, l_date};
}
std::tuple<bool, chrono::year_month_day> url_route_component_t::component_base_t::convert_year_month_day(
    const std::string& in_str
) const {
  std::smatch l_result{};
  chrono::year_month_day l_date{};
  if (std::regex_match(in_str, l_result, regex_)) {
    for (auto& l_item : l_result) {
      std::istringstream l_stream{l_item.str()};
      l_stream >> chrono::parse("%Y-%m-%d", l_date);
      return {true, l_date};
    }
  }
  return {false, l_date};
}

std::shared_ptr<void> url_route_component_t::create_object() const {
  if (create_object_) {
    return create_object_();
  }
  return {};
}

url_route_component_t& url_route_component_t::operator/(const std::shared_ptr<component_base_t>& in_ptr) {
  if (!create_object_) {
    create_object_ = in_ptr->create_object();
    object_type_   = in_ptr->get_type();
  }
  if (object_type_ != in_ptr->get_type()) throw std::runtime_error("url route component type mismatch");

  component_vector_.push_back(in_ptr);
  return *this;
}

void http_function_base_t::websocket_init(session_data_ptr in_handle) {}
boost::asio::awaitable<void> http_function_base_t::websocket_callback(
    boost::beast::websocket::stream<tcp_stream_type> in_stream, session_data_ptr in_handle
) {
  co_return;
}
bool http_function_base_t::has_websocket() const { return false; }
bool http_function_base_t::is_proxy() const { return false; }

std::tuple<bool, std::shared_ptr<void>> http_function::set_match_url(boost::urls::segments_ref in_segments_ref) const {
  std::map<std::string, std::string> l_str{};

  std::vector<std::string> l_segments_ref_not_null = in_segments_ref |
                                                     ranges::views::filter([](auto&& i) { return !i.empty(); }) |
                                                     ranges::to<std::vector<std::string>>;

  if (l_segments_ref_not_null.size() != url_route_.component_vector().size()) {
    return {false, {}};
  }
  auto l_data = url_route_.create_object();
  for (const auto& [l_cap, l_seg] : ranges::zip_view(url_route_.component_vector(), l_segments_ref_not_null)) {
    if (!l_cap->set(l_seg, l_data)) return {false, l_data};
  }
  return {true, l_data};
}
const std::type_info& http_function::get_type() const { return typeid(void); }
void http_function::check_type() const {
  if (url_route_.object_type() != get_type()) throw std::runtime_error("url route component type mismatch");
}

}  // namespace doodle::http
