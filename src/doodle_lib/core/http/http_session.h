//
// Created by TD on 2024/2/20.
//

#pragma once
#include <doodle_core/doodle_core_fwd.h>
namespace doodle::http {

class http_session {
  entt::handle handle_;

 public:
  explicit http_session(entt::handle in_handle) : handle_(std::move(in_handle)){};

  void run();
};

}  // namespace doodle::http