//
// Created by TD on 24-9-26.
//

#include "kitsu_front_end_reg.h"

#include <doodle_lib/core/http/zlib_deflate_file_body.h>
#include <doodle_lib/http_method/kitsu/kitsu.h>
#include <doodle_lib/http_method/kitsu/kitsu_front_end.h>
namespace doodle::http {
namespace {

FSys::path make_doc_path(const std::shared_ptr<FSys::path>& in_root, const boost::urls::segments_ref& in_) {
  auto l_path = *in_root;
  for (auto&& i : in_) {
    l_path /= i;
  }
  if (in_.size() == 0) {
    l_path /= "index.html";
  }
  if (!FSys::exists(l_path)) {
    l_path = *in_root / "index.html";
  }
  return l_path;
}

boost::asio::awaitable<boost::beast::http::message_generator> get_files(
    std::shared_ptr<FSys::path> in_root, http::session_data_ptr in_handle
) {
  auto l_path = make_doc_path(in_root, in_handle->url_.segments());
  auto l_ext  = l_path.extension();
  boost::system::error_code l_code{};
  auto l_set_handle = [&l_ext, &in_handle](auto&& in_res) {
    in_res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
    in_res.set(boost::beast::http::field::content_type, kitsu::mime_type(l_ext));
    in_res.keep_alive(in_handle->keep_alive_);
    in_res.prepare_payload();
  };

  if (in_handle->req_header_[boost::beast::http::field::accept_encoding].contains("deflate")) {
    boost::beast::http::response<http::zlib_deflate_file_body> l_res{
        boost::beast::http::status::ok, in_handle->version_
    };
    l_res.body().open(l_path.generic_string().c_str(), std::ios::in | std::ios::binary, l_code);
    l_res.set(boost::beast::http::field::content_encoding, "deflate");
    l_set_handle(l_res);
    if (l_code)
      co_return in_handle->make_error_code_msg(boost::beast::http::status::service_unavailable, l_code.message());
    co_return std::move(l_res);
  }

  boost::beast::http::response<boost::beast::http::file_body> l_res{
      boost::beast::http::status::ok, in_handle->version_
  };
  l_res.body().open(l_path.generic_string().c_str(), boost::beast::file_mode::scan, l_code);
  l_set_handle(l_res);
  if (l_code)
    co_return in_handle->make_error_code_msg(boost::beast::http::status::service_unavailable, l_code.message());
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
  l_res.set(boost::beast::http::field::content_type, kitsu::mime_type(l_path.extension()));
  l_res.content_length(FSys::file_size(l_path));
  l_res.keep_alive(in_handle->keep_alive_);
  l_res.prepare_payload();
  co_return std::move(l_res);
}
}  // namespace

void reg_kitsu_front_end_http(kitsu::http_route_proxy& in_route, const FSys::path& in_root) {
  auto l_path = std::make_shared<FSys::path>(in_root);
  default_logger_raw()->warn("初始化前端路径 {}", in_root);
  doodle::kitsu::kitsu_front_end l_end{
      FSys::path{in_root}, boost::beast::http::verb::get, std::bind_front(get_files, l_path)
  };
  in_route.reg_front_end(
      std::make_shared<doodle::kitsu::kitsu_front_end>(
          FSys::path{in_root}, boost::beast::http::verb::get, std::bind_front(get_files, l_path)
      ),
      std::make_shared<doodle::kitsu::kitsu_front_end>(
          FSys::path{in_root}, boost::beast::http::verb::head, std::bind_front(get_files_head, l_path)
      )
  );
}
}  // namespace doodle::http