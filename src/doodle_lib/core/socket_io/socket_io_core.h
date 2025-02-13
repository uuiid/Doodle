//
// Created by TD on 25-2-12.
//

#pragma once
#include <doodle_core/doodle_core_fwd.h>

#include <boost/signals2/signal.hpp>

namespace doodle::socket_io {
class sid_ctx;
/// socket 连接
class socket_io_core : public std::enable_shared_from_this<socket_io_core> {
  uuid sid_;
  std::shared_ptr<sid_ctx> ctx_;
  std::string namespace_;
  std::shared_ptr<boost::signals2::signal<void(const std::string&, const nlohmann::json&)>> on_message_;

 public:
  explicit socket_io_core(
      const std::shared_ptr<sid_ctx>& in_ctx, const std::string& in_namespace, const nlohmann::json& in_json
  );
  // destructor
  ~socket_io_core()                                      = default;
  // copy constructor
  socket_io_core(const socket_io_core& other)            = default;
  socket_io_core& operator=(const socket_io_core& other) = default;
  // move constructor
  socket_io_core(socket_io_core&& other)                 = default;
  socket_io_core& operator=(socket_io_core&& other)      = default;

  const uuid& get_sid() const { return sid_; }
  const std::string& get_namespace() const { return namespace_; }
  nlohmann::json auth_{};

  void emit(const std::string& in_event, const nlohmann::json& in_data);
  template <typename Solt>
  auto on_message(Solt&& in_solt) {
    return on_message_->connect(in_solt);
  }
};
}  // namespace doodle::socket_io
