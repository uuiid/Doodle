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
  if (!in_.empty() && in_.front() != "api" && !FSys::exists(l_path)) {
    l_path = *in_root / "index.html";
  }
  return l_path;
}

class get_files_kitsu_front_end : public doodle::kitsu::kitsu_front_end {
 public:
  explicit get_files_kitsu_front_end(const std::shared_ptr<FSys::path>& in_root)
      : doodle::kitsu::kitsu_front_end(boost::beast::http::verb::get) {
    root_path_ = in_root;
  }
  boost::asio::awaitable<boost::beast::http::message_generator> callback(session_data_ptr in_handle) override {
    auto l_path = make_doc_path(root_path_, in_handle->url_.segments());
    co_return in_handle->make_msg(l_path, kitsu::mime_type(l_path.extension()));
  }
};
class get_files_head_kitsu_front_end : public doodle::kitsu::kitsu_front_end {
 public:
  explicit get_files_head_kitsu_front_end(const std::shared_ptr<FSys::path>& in_root)
      : doodle::kitsu::kitsu_front_end(boost::beast::http::verb::head) {
    root_path_ = in_root;
  }
  boost::asio::awaitable<boost::beast::http::message_generator> callback(session_data_ptr in_handle) override {
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
};

}  // namespace

void reg_kitsu_front_end_http(kitsu::http_route_proxy& in_route, const FSys::path& in_root) {
  auto l_path = std::make_shared<FSys::path>(in_root);
  default_logger_raw()->warn("初始化前端路径 {}", in_root);
  in_route.reg_front_end(
      std::make_shared<get_files_kitsu_front_end>(l_path), std::make_shared<get_files_head_kitsu_front_end>(l_path)
  );
}
}  // namespace doodle::http