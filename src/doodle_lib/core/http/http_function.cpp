//
// Created by TD on 2024/2/21.
//

#include "http_function.h"

#include <doodle_core/lib_warp/boost_fmt_error.h>
#include <doodle_core/logger/logger.h>

#include <doodle_lib/core/http/http_session_data.h>
namespace doodle::http {
void url_route_component_t::initializer_t::parse_url_path() {
  std::vector<std::string> l_result{};
  boost::split(l_result, url_path_, boost::is_any_of("/"));
  // l_result |= ranges::actions::remove_if([](const auto& in_str) { return in_str.empty(); });
  for (auto& l_item : l_result) {
    if (l_item.empty()) continue;
    auto l_is = l_item.find("{}") != std::string::npos;
    component_vector_.emplace_back(
        com{.str_        = l_is ? std::move(l_item) : std::string{},
            .is_capture_ = l_is,
            .obj_        = l_is ? nullptr : std::make_shared<component_base_t>(std::move(l_item))}
    );
    capture_count_ += l_is;
  }
  next_capture();
}
void url_route_component_t::initializer_t::next_capture() {
  if (pos_ == component_vector_.size()) throw_exception(doodle_error{"invalid url route"});
  pos_ = std::distance(
      component_vector_.begin(),
      std::ranges::find_if(
          component_vector_.begin() + pos_, component_vector_.end(), [](com& in_com) { return in_com.is_capture_; }
      )
  );
}
std::vector<std::shared_ptr<url_route_component_t::component_base_t>>
url_route_component_t::initializer_t::get_component_vector() const {
  std::vector<std::shared_ptr<url_route_component_t::component_base_t>> l_result{};
  for (auto& l_item : component_vector_) {
    BOOST_ASSERT(l_item.obj_);
    l_result.push_back(l_item.obj_);
  }
  return l_result;
}
url_route_component_t::initializer_t::operator url_route_component_t() const {
  auto l_it     = std::ranges::find_if(component_vector_, [](const com& in_com) { return in_com.is_capture_; });
  auto l_is_end = l_it == component_vector_.end();
  url_route_component_t l_result{
      get_component_vector(), l_is_end ? std::function<std::shared_ptr<void>()>{} : l_it->obj_->create_object(),
      l_is_end ? typeid(void) : l_it->obj_->get_type()
  };
  return l_result;
}

url_route_component_t::initializer_t operator""_url(char const* in_str, std::size_t in_len) {
  url_route_component_t::initializer_t l_url{in_str, in_len};
  l_url.parse_url_path();
  return l_url;
}

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
    for (auto l_begin = ++l_result.begin(), l_end = l_result.end(); l_begin != l_end; ++l_begin) {
      return {true, from_uuid_str(l_begin->str())};
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
    for (auto l_begin = ++l_result.begin(), l_end = l_result.end(); l_begin != l_end; ++l_begin) {
      std::istringstream l_stream{l_begin->str()};
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
    for (auto l_begin = ++l_result.begin(), l_end = l_result.end(); l_begin != l_end; ++l_begin) {
      std::istringstream l_stream{l_begin->str()};
      l_stream >> chrono::parse("%Y-%m-%d", l_date);
      return {true, l_date};
    }
  }
  return {false, l_date};
}
std::tuple<bool, std::int32_t> url_route_component_t::component_base_t::convert_number(
    const std::string& in_str
) const {
  std::smatch l_result{};
  if (std::regex_match(in_str, l_result, regex_)) {
    for (auto l_begin = ++l_result.begin(), l_end = l_result.end(); l_begin != l_end; ++l_begin) {
      return {true, std::stoi(l_begin->str())};
    }
  }
  return {false, 0};
}
std::tuple<bool, FSys::path> url_route_component_t::component_base_t::convert_file_name(
    const std::string& in_str
) const {
  std::smatch l_result{};
  if (std::regex_match(in_str, l_result, regex_)) {
    for (auto l_begin = ++l_result.begin(), l_end = l_result.end(); l_begin != l_end; ++l_begin) {
      return {true, FSys::path{l_begin->str()}};
    }
  }
  return {false, {}};
}

void http_function::websocket_init(session_data_ptr in_handle) {}
void http_function::websocket_callback(
    boost::beast::websocket::stream<tcp_stream_type> in_stream, session_data_ptr in_handle
) {
  return;
}
bool http_function::has_websocket() const { return false; }

std::tuple<bool, std::shared_ptr<http_function>> url_route_component_t::set_match_url(
    boost::urls::segments_ref in_segments_ref, const std::shared_ptr<http_function>& in_data
) const {
  std::map<std::string, std::string> l_str{};

  std::vector<std::string> l_segments_ref_not_null = in_segments_ref |
                                                     ranges::views::filter([](auto&& i) { return !i.empty(); }) |
                                                     ranges::to<std::vector<std::string>>;

  if (l_segments_ref_not_null.size() != component_vector().size()) {
    return {false, {}};
  }
  for (const auto& [l_cap, l_seg] : ranges::zip_view(component_vector(), l_segments_ref_not_null)) {
    if (!l_cap->set(l_seg, in_data)) return {false, {}};
  }
  return {true, in_data->clone()};
}

boost::asio::awaitable<boost::beast::http::message_generator> http_function::callback(session_data_ptr in_handle) {
  switch (in_handle->method()) {
    case boost::beast::http::verb::get:
      return get(in_handle);
    case boost::beast::http::verb::put:
      return put(in_handle);
    case boost::beast::http::verb::post:
      return post(in_handle);
    case boost::beast::http::verb::options:
      return options(in_handle);
    case boost::beast::http::verb::delete_:
      return delete_(in_handle);
    default:
      return other_callback(in_handle);
  }
}

boost::asio::awaitable<boost::beast::http::message_generator> http_function::other_callback(
    session_data_ptr in_handle
) {
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

}  // namespace doodle::http
