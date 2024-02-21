//
// Created by TD on 2024/2/20.
//

#pragma once
#include <doodle_core/doodle_core_fwd.h>
namespace doodle::http {

class http_session {
  entt::handle handle_;

  void do_read(boost::system::error_code ec);

 public:
  explicit http_session(entt::handle in_handle) : handle_(std::move(in_handle)){};

  void run();

  void do_close();
  void operator()(boost::system::error_code ec, std::size_t bytes_transferred);
};

}  // namespace doodle::http