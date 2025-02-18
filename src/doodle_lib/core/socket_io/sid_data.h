//
// Created by TD on 25-2-17.
//

#pragma once
#include <doodle_core/core/core_set.h>
#include <doodle_core/doodle_core_fwd.h>

#include <boost/signals2.hpp>
namespace doodle::socket_io {
class sid_ctx;
class socket_io_core;
struct socket_io_packet;
class socket_io_websocket_core;
using socket_io_packet_ptr          = std::shared_ptr<socket_io_packet>;
using socket_io_core_ptr            = std::shared_ptr<socket_io_core>;
using socket_io_websocket_core_wptr = std::weak_ptr<socket_io_websocket_core>;
using socket_io_websocket_core_ptr  = std::shared_ptr<socket_io_websocket_core>;

class sid_data {
 public:
  explicit sid_data(sid_ctx* in_ctx, uuid in_sid = core_set::get_set().get_uuid())
      : ctx_{in_ctx},
        sid_{in_sid},
        last_time_{std::chrono::system_clock::now()},
        is_upgrade_to_websocket_{false},
        close_{false} {}

  bool is_upgrade_to_websocket() const;
  bool is_timeout() const;
  void update_sid_time();
  void upgrade_to_websocket() { is_upgrade_to_websocket_ = true; }
  void close();
  uuid get_sid() const { return sid_; }

  std::shared_ptr<void> get_lock();
  bool is_locked() const;
  //
  // std::shared_ptr<socket_io_core> generate_socket_io_core(
  //     const std::string& in_namespace, const nlohmann::json& in_json
  // );
  std::string parse_socket_io(socket_io_packet& in_code);

  boost::asio::awaitable<std::string> async_event();

  void set_websocket_connect(const socket_io_websocket_core_ptr& in_websocket);

 private:
  struct lock_type {
    sid_data* data_;
    explicit lock_type(sid_data* in_data) : data_{in_data} { ++data_->lock_count_; }
    ~lock_type() { --data_->lock_count_; }
  };
  friend class sid_ctx;
  sid_ctx* ctx_;
  const uuid sid_;
  std::atomic<chrono::sys_time_pos> last_time_;
  std::atomic_bool is_upgrade_to_websocket_;
  std::atomic_int lock_count_;
  std::atomic_bool close_;

  std::map<std::string, socket_io_core_ptr> socket_io_contexts_;
  boost::signals2::signal<void(const std::string&)> socket_io_signal_{};
};

}  // namespace doodle::socket_io
