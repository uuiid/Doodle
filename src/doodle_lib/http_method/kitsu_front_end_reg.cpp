//
// Created by TD on 24-9-26.
//

#include "kitsu_front_end_reg.h"

#include <doodle_lib/http_method/kitsu/kitsu_front_end.h>
namespace doodle::http {
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
  if (in_ext == ".map") return "application/json";
}

FSys::path make_doc_path(const std::shared_ptr<FSys::path>& in_root, const boost::urls::segments_ref& in_) {
  auto l_path = *in_root;
  for (auto&& i : in_) {
    l_path /= i;
  }
  if (in_.size() == 0) {
    l_path /= "index.html";
  }
  return l_path;
}

boost::asio::awaitable<boost::beast::http::message_generator> get_files(
    std::shared_ptr<FSys::path> in_root, http::session_data_ptr in_handle
) {
  auto l_path = make_doc_path(in_root, in_handle->url_.segments());
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

boost::asio::awaitable<boost::beast::http::message_generator> get_files_head(
    std::shared_ptr<FSys::path> in_root, http::session_data_ptr in_handle
) {
  auto l_path = make_doc_path(in_root, in_handle->url_.segments());
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
}  // namespace

void reg_kitsu_front_end_http(http_route& in_route, const FSys::path& in_root) {
  auto l_path = std::make_shared<FSys::path>(in_root);
  default_logger_raw()->warn("初始化前端路径 {}", in_root);
  kitsu::kitsu_front_end l_end{FSys::path{in_root}, boost::beast::http::verb::get, std::bind_front(get_files, l_path)};
  in_route
      .reg(std::make_shared<kitsu::kitsu_front_end>(
          FSys::path{in_root}, boost::beast::http::verb::get, std::bind_front(get_files, l_path)
      ))
      .reg(std::make_shared<kitsu::kitsu_front_end>(
          FSys::path{in_root}, boost::beast::http::verb::head, std::bind_front(get_files_head, l_path)
      ));
}
}  // namespace doodle::http