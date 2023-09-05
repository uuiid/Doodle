//
// Created by td_main on 2023/9/5.
//
#pragma once
#include <doodle_lib/doodle_lib_fwd.h>

#include <boost/beast.hpp>
namespace doodle {
namespace render_farm {
namespace detail {

class post_log {
 public:
  using str_parser_t   = boost::beast::http::request_parser<boost::beast::http::string_body>;
  using str_parser_ptr = std::shared_ptr<str_parser_t>;
  enum class log_enum : std::int32_t {
    log,
    err,
  };

 private:
  struct impl_t {
    entt::handle session_handle_;
    entt::handle msg_handle_;
    log_enum msg_type_;
    str_parser_ptr parser_;
  };
  std::shared_ptr<impl_t> impl_;

 public:
  explicit post_log(entt::handle in_session, entt::handle in_msg_handle, log_enum in_msg_type, str_parser_ptr in_parser)
      : impl_(std::make_shared<impl_t>()) {
    impl_->session_handle_ = std::move(in_session);
    impl_->msg_handle_     = std::move(in_msg_handle);
    impl_->msg_type_       = in_msg_type;
    impl_->parser_         = std::move(in_parser);
  }

  ~post_log() = default;


  void operator()(boost::system::error_code ec, std::size_t bytes_transferred) const;
};

}  // namespace detail
}  // namespace render_farm
}  // namespace doodle
