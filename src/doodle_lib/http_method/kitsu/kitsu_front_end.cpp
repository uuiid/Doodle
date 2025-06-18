//
// Created by TD on 24-9-26.
//

#include "kitsu_front_end.h"

#include <doodle_lib/core/http/http_session_data.h>
#include <doodle_lib/core/http/zlib_deflate_file_body.h>
#include <doodle_lib/http_method/kitsu/kitsu.h>
namespace doodle::http {
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

namespace {
FSys::path make_doc_path(const std::shared_ptr<FSys::path>& in_root, const boost::urls::segments_ref& in_) {
  auto l_path = *in_root;
  for (auto&& i : in_) {
    l_path /= i;
  }
  if (in_.size() == 0) {
    l_path /= "index.html";
  }
  if (!in_.empty() && in_.front() != "api" && !FSys::exists(l_path)) {
    l_path = *in_root / "index.html";
  }
  return l_path;
}
}  // namespace
boost::asio::awaitable<boost::beast::http::message_generator> get_files_kitsu_front_end::callback(
    http::session_data_ptr in_handle
) {
  auto l_path = make_doc_path(root_path_, in_handle->url_.segments());
  co_return in_handle->make_msg(l_path, kitsu::mime_type(l_path.extension()));
}
boost::asio::awaitable<boost::beast::http::message_generator> get_files_head_kitsu_front_end::callback(
    session_data_ptr in_handle
) {
  auto l_path = make_doc_path(root_path_, in_handle->url_.segments());
  boost::beast::http::response<boost::beast::http::file_body> l_res{
      boost::beast::http::status::ok, in_handle->version_
  };
  l_res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
  l_res.set(boost::beast::http::field::content_type, kitsu::mime_type(l_path.extension()));
  l_res.content_length(FSys::file_size(l_path));
  l_res.keep_alive(in_handle->keep_alive_);
  l_res.prepare_payload();
  co_return std::move(l_res);
}
}  // namespace doodle::http