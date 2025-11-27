//
// Created by TD on 2024/2/21.
//

#include "http_function.h"

#include <doodle_core/lib_warp/boost_fmt_error.h>
#include <doodle_core/logger/logger.h>

#include <doodle_lib/core/http/http_session_data.h>

#include <boost/algorithm/string.hpp>
#include <boost/range/iterator_range.hpp>
namespace doodle::http {
void url_route_component_t::initializer_t::parse_url_path() {
  std::vector<std::string> l_result{};
  boost::split(l_result, url_path_, boost::is_any_of("/"));
  while (url_path_.find("//") != std::string::npos) boost::replace_all(url_path_, "//", "/");

  if (url_path_.front() != '/') url_path_ = "/" + url_path_;
  std::vector<boost::iterator_range<std::string::iterator>> l_ranges{};
  boost::algorithm::iter_find(l_ranges, url_path_, boost::algorithm::first_finder("{}"));
  capture_count_ = l_ranges.size();
  // 正则转义
  boost::replace_all(url_path_, "/", R"(\/)");
  boost::replace_all(url_path_, ".", R"(\.)");
  boost::replace_all(url_path_, "?", R"(\?)");

}
url_route_component_t::component_vector_t url_route_component_t::initializer_t::get_component_vector() const {
  return component_;
}
url_route_component_t::initializer_t::operator url_route_component_ptr() const {
  return std::make_shared<url_route_component_t>(url_path_, get_component_vector());
}
struct initializer_url_list {
  std::set<std::string> url_list{};
};
url_route_component_t::initializer_t operator""_url(char const* in_str, std::size_t in_len) {
  if (!g_ctx().contains<initializer_url_list>()) g_ctx().emplace<initializer_url_list>(initializer_url_list{});
  auto& l_list = g_ctx().get<initializer_url_list>();
  std::string l_str{in_str, in_len};
  if (l_list.url_list.count(l_str)) throw_exception(doodle_error{"url 已经注册 {}", l_str});
  l_list.url_list.insert(l_str);

  url_route_component_t::initializer_t l_url{in_str, in_len};
  l_url.parse_url_path();
  return l_url;
}

uuid url_route_component_t::component_base_t::convert_uuid(const std::string& in_str) const {
  return from_uuid_str(in_str);
}
chrono::year_month url_route_component_t::component_base_t::convert_year_month(const std::string& in_str) const {
  chrono::year_month l_date{};
  std::istringstream l_stream{in_str};
  l_stream >> chrono::parse("%Y-%m", l_date);
  return l_date;
}
chrono::year_month_day url_route_component_t::component_base_t::convert_year_month_day(
    const std::string& in_str
) const {
  chrono::year_month_day l_date{};
  std::istringstream l_stream{in_str};
  l_stream >> chrono::parse("%Y-%m-%d", l_date);
  return l_date;
}
std::int32_t url_route_component_t::component_base_t::convert_number(const std::string& in_str) const {
  return std::stoi(in_str);
}
FSys::path url_route_component_t::component_base_t::convert_file_name(const std::string& in_str) const {
  return FSys::path{in_str};
}

std::tuple<bool, std::shared_ptr<http_function>> url_route_component_t::set_match_url(
    boost::urls::segments_ref in_segments_ref, const std::shared_ptr<http_function>& in_data
) const {
  std::smatch l_result{};
  auto l_url_path = in_segments_ref.url().path();
  if (l_url_path.back() == '/') {
    // 去掉末尾的斜杠
    l_url_path.pop_back();
  }
  // 检查路径是否匹配正则表达式
  if (!std::regex_match(l_url_path, l_result, url_regex_)) {
    return {false, {}};
  }
  auto l_ptr           = in_data->clone();
  std::int32_t l_index = 0;
  for (auto l_begin = ++l_result.begin(), l_end = l_result.end(); l_begin != l_end; ++l_begin, ++l_index) {
    component_vector().at(l_index)->set(l_begin->str(), l_ptr);
  }

  return {true, l_ptr};
}

void http_function::websocket_init(session_data_ptr in_handle) {}
void http_function::websocket_callback(
    boost::beast::websocket::stream<tcp_stream_type> in_stream, session_data_ptr in_handle
) {}
bool http_function::has_websocket() const { return false; }
void http_function::parse_header(const session_data_ptr& in_handle) {}

boost::asio::awaitable<boost::beast::http::message_generator> http_function::callback(session_data_ptr in_handle) {
  parse_header(in_handle);
  switch (in_handle->method()) {
    case boost::beast::http::verb::delete_:
      return delete_(in_handle);
    case boost::beast::http::verb::get:
      return get(in_handle);
    case boost::beast::http::verb::head:
      return head(in_handle);
    case boost::beast::http::verb::post:
      return post(in_handle);
    case boost::beast::http::verb::put:
      return put(in_handle);
    case boost::beast::http::verb::options:
      return options(in_handle);
    case boost::beast::http::verb::patch:
      return patch(in_handle);
    default:
      return other_callback(in_handle);
  }
  return other_callback(in_handle);
}

boost::asio::awaitable<boost::beast::http::message_generator> http_function::other_callback(
    session_data_ptr in_handle
) {
  co_return in_handle->make_error_code_msg(boost::beast::http::status::not_found, "服务器端未实现 api");
}
boost::asio::awaitable<boost::beast::http::message_generator> http_function::get(session_data_ptr in_handle) {
  co_return in_handle->make_error_code_msg(boost::beast::http::status::not_found, "服务器端未实现 api");
}
boost::asio::awaitable<boost::beast::http::message_generator> http_function::put(session_data_ptr in_handle) {
  co_return in_handle->make_error_code_msg(boost::beast::http::status::not_found, "服务器端未实现 api");
}
boost::asio::awaitable<boost::beast::http::message_generator> http_function::post(session_data_ptr in_handle) {
  co_return in_handle->make_error_code_msg(boost::beast::http::status::not_found, "服务器端未实现 api");
}
boost::asio::awaitable<boost::beast::http::message_generator> http_function::options(session_data_ptr in_handle) {
  co_return in_handle->make_msg(std::string{});
}
boost::asio::awaitable<boost::beast::http::message_generator> http_function::delete_(session_data_ptr in_handle) {
  co_return in_handle->make_error_code_msg(boost::beast::http::status::not_found, "服务器端未实现 api");
}
boost::asio::awaitable<boost::beast::http::message_generator> http_function::head(session_data_ptr in_handle) {
  co_return in_handle->make_error_code_msg(boost::beast::http::status::not_found, "服务器端未实现 api");
}
boost::asio::awaitable<boost::beast::http::message_generator> http_function::patch(session_data_ptr in_handle) {
  co_return in_handle->make_error_code_msg(boost::beast::http::status::not_found, "服务器端未实现 api");
}
}  // namespace doodle::http
