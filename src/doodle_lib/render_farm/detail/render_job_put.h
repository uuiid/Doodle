//
// Created by td_main on 2023/8/17.
//
#pragma once
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/render_farm/detail/basic_json_body.h>

#include <boost/beast.hpp>
namespace doodle {
namespace render_farm {
namespace detail {

class render_job_put {
 public:
  using parser_type     = boost::beast::http::request_parser<basic_json_body>;
  using parser_type_ptr = std::shared_ptr<parser_type>;
  explicit render_job_put(entt::handle in_handle, parser_type_ptr in_parser_type)
      : ptr_{std::make_shared<data_type>()} {
    ptr_->handle_ = std::move(in_handle);
    ptr_->parser_ = std::move(in_parser_type);
  }
  ~render_job_put() = default;

  void operator()(boost::system::error_code ec, std::size_t bytes_transferred);

 private:
  struct data_type {
    entt::handle handle_;
    std::shared_ptr<parser_type> parser_;
    boost::beast::flat_buffer buffer_;
  };
  std::shared_ptr<data_type> ptr_;
};

}  // namespace detail
}  // namespace render_farm
}  // namespace doodle
