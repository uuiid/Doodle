//
// Created by TD on 25-2-12.
//

#pragma once
#include <doodle_core/doodle_core_fwd.h>

namespace doodle::socket_io {
class sid_ctx;
/// socket 连接
class socket_io_core : public std::enable_shared_from_this<socket_io_core> {
  uuid sid_;
  std::shared_ptr<sid_ctx> ctx_;
  std::string namespace_;

 public:
  explicit socket_io_core(const std::shared_ptr<sid_ctx>& in_ctx, const std::string& in_namespace);

  uuid get_sid() const { return sid_; }
  nlohmann::json auth_{};

  void emit(const std::string& in_event, const nlohmann::json& in_data);
};
}  // namespace doodle::socket_io
