//
// Created by TD on 24-9-26.
//

#include "kitsu_front_end.h"

#include <doodle_lib/core/http/http_session_data.h>
namespace doodle::kitsu {
namespace {
std::string_view mime_type(const FSys::path& in_ext) {
  if (in_ext == ".htm") return "text/html";
  if (in_ext == ".html") return "text/html";
  if (in_ext == ".php") return "text/html";
  if (in_ext == ".css") return "text/css";
  if (in_ext == ".txt") return "text/plain";
  if (in_ext == ".js") return "application/javascript";
  if (in_ext == ".json") return "application/json";
  if (in_ext == ".xml") return "application/xml";
  if (in_ext == ".swf") return "application/x-shockwave-flash";
  if (in_ext == ".flv") return "video/x-flv";
  if (in_ext == ".png") return "image/png";
  if (in_ext == ".jpe") return "image/jpeg";
  if (in_ext == ".jpeg") return "image/jpeg";
  if (in_ext == ".jpg") return "image/jpeg";
  if (in_ext == ".gif") return "image/gif";
  if (in_ext == ".bmp") return "image/bmp";
  if (in_ext == ".ico") return "image/vnd.microsoft.icon";
  if (in_ext == ".tiff") return "image/tiff";
  if (in_ext == ".tif") return "image/tiff";
  if (in_ext == ".svg") return "image/svg+xml";
  if (in_ext == ".svgz") return "image/svg+xml";
}
}  // namespace
boost::asio::awaitable<boost::beast::http::message_generator> kitsu_front_end::get_files(
    http::session_data_ptr in_handle
) const {
  auto l_path = root_path_ / std::string{in_handle->url_.segments().buffer()};
  auto l_ext  = l_path.extension();
  boost::system::error_code l_code{};
  boost::beast::http::response<boost::beast::http::file_body> l_res{
      boost::beast::http::status::ok, in_handle->version_
  };
  l_res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
  l_res.set(boost::beast::http::field::content_type, mime_type(l_ext));
  l_res.body().open(l_path.generic_string().c_str(), boost::beast::file_mode::scan, l_code);
  if (l_code)
    co_return in_handle->make_error_code_msg(boost::beast::http::status::service_unavailable, l_code.message());
  l_res.keep_alive(in_handle->keep_alive_);
  l_res.prepare_payload();
  co_return std::move(l_res);
}

std::tuple<bool, http::capture_t> kitsu_front_end::set_match_url(boost::urls::segments_ref in_segments_ref) const {
  if (FSys::exists(root_path_ / std::string{in_segments_ref.buffer()})) {
    return {true, http::capture_t{}};
  }
  return {false, http::capture_t{}};
}
boost::asio::awaitable<boost::beast::http::message_generator> kitsu_front_end_head::get_files(
    http::session_data_ptr in_handle
) const {
  auto l_path = root_path_ / std::string{in_handle->url_.segments().buffer()};
  boost::beast::http::response<boost::beast::http::file_body> l_res{
      boost::beast::http::status::ok, in_handle->version_
  };
  l_res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
  l_res.set(boost::beast::http::field::content_type, mime_type(l_path.extension()));
  try {
    l_res.content_length(FSys::file_size(l_path));
  } catch (...) {
    co_return in_handle->make_error_code_msg(
        boost::beast::http::status::service_unavailable, boost::current_exception_diagnostic_information()
    );
  }
  l_res.keep_alive(in_handle->keep_alive_);
  l_res.prepare_payload();
  co_return std::move(l_res);
}

}  // namespace doodle::kitsu