//
// Created by td_main on 2023/8/14.
//
#pragma once
#include <doodle_lib/doodle_lib_fwd.h>

#include <boost/beast.hpp>
namespace doodle {
namespace render_farm {
namespace detail {
// 转发ue任务到服务器
class forward_to_server {
 public:
  using request_parser_ptr = std::shared_ptr<boost::beast::http::request_parser<boost::beast::http::string_body>>;
  using request_ptr        = std::shared_ptr<boost::beast::http::request<boost::beast::http::string_body>>;
  explicit forward_to_server(entt::handle in_self, request_parser_ptr in_parser) : ptr_{std::make_shared<data_type>()} {
    ptr_->self_handle_ = in_self;
    ptr_->parser_ = std::move(in_parser);
  }
  ~forward_to_server() = default;
  void operator()(boost::system::error_code ec, std::size_t bytes_transferred);

 private:
  struct data_type {
    entt::handle self_handle_;
    request_parser_ptr parser_;
    request_ptr request_;
    std::shared_ptr<boost::beast::tcp_stream> stream_;
    boost::beast::http::response<boost::beast::http::string_body> response_;
    boost::beast::flat_buffer buffer_;
  };
  std::shared_ptr<data_type> ptr_;

  void on_write(boost::system::error_code ec, std::size_t bytes_transferred);
  void on_read(boost::system::error_code ec, std::size_t bytes_transferred);
};

}  // namespace detail
}  // namespace render_farm
}  // namespace doodle
