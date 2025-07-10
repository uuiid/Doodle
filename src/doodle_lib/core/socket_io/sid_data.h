//
// Created by TD on 25-2-17.
//

#pragma once
#include <doodle_core/core/core_set.h>
#include <doodle_core/doodle_core_fwd.h>

#include <boost/asio/experimental/concurrent_channel.hpp>
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

class sid_data : public std::enable_shared_from_this<sid_data> {
  using channel_type = boost::asio::experimental::concurrent_channel<void(boost::system::error_code, std::string)>;

 public:
  explicit sid_data(sid_ctx* in_ctx, uuid in_sid = core_set::get_set().get_uuid())
      : ctx_{in_ctx},
        sid_{in_sid},
        last_time_{std::chrono::system_clock::now()},
        is_upgrade_to_websocket_{false},
        close_{false},
        channel_(g_io_context()) {}

  bool is_upgrade_to_websocket() const;
  bool is_timeout() const;
  void update_sid_time();
  void upgrade_to_websocket() { is_upgrade_to_websocket_ = true; }
  void close();
  uuid get_sid() const { return sid_; }

  std::shared_ptr<void> get_lock();
  bool is_locked() const;
  // 开始 ping pong 消息发送
  void run();
  void handle_socket_io(socket_io_packet& in_code);
  /// 处理 engine io 包, 结束处理返回 true, 继续处理返回 false
  bool handle_engine_io(std::string& in_data);

  boost::asio::awaitable<std::string> async_event();

  void set_websocket_connect(const socket_io_websocket_core_ptr& in_websocket);

 private:
  boost::asio::awaitable<void> impl_run();
  void seed_message(const std::string& in_message);
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
  socket_io_websocket_core_wptr websocket_;

  channel_type channel_;

  std::map<std::string, socket_io_core_ptr> socket_io_contexts_;
};

}  // namespace doodle::socket_io
