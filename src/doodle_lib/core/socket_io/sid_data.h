//
// Created by TD on 25-2-17.
//

#pragma once
#include <doodle_core/core/core_set.h>
#include <doodle_core/doodle_core_fwd.h>

#include <boost/asio/experimental/concurrent_channel.hpp>
#include <boost/signals2.hpp>

#include <atomic>

namespace doodle::socket_io {
struct packet_base;
class sid_ctx;
class socket_io_core;
struct socket_io_packet;
class socket_io_websocket_core;
using socket_io_packet_ptr          = std::shared_ptr<socket_io_packet>;
using socket_io_core_ptr            = std::shared_ptr<socket_io_core>;
using socket_io_websocket_core_wptr = std::weak_ptr<socket_io_websocket_core>;
using socket_io_websocket_core_ptr  = std::shared_ptr<socket_io_websocket_core>;

class sid_data : public std::enable_shared_from_this<sid_data> {
  using channel_type =
      boost::asio::experimental::concurrent_channel<void(boost::system::error_code, std::shared_ptr<packet_base>)>;

 public:
  explicit sid_data(sid_ctx* in_ctx, uuid in_sid = core_set::get_set().get_uuid())
      : ctx_{in_ctx},
        sid_{in_sid},
        last_time_{std::chrono::system_clock::now()},
        is_upgrade_to_websocket_{false},
        close_{false},
        channel_(g_io_context(), 9999),
        block_message_() {}

  bool is_upgrade_to_websocket() const;
  bool is_timeout() const;
  void update_sid_time();
  void upgrade_to_websocket() { is_upgrade_to_websocket_ = true; }
  void close();
  const uuid& get_sid() const { return sid_; }

  std::shared_ptr<void> get_lock();
  bool is_locked() const;
  // 开始 ping pong 消息发送
  void run();
  boost::asio::awaitable<void> handle_socket_io(socket_io_packet& in_code);
  /// 处理 engine io 包, 结束处理返回 true, 继续处理返回 false,
  /// 第二个参数是必须立即返回的 engine io 包, 这个包的优先级最高, 不进入队列
  std::tuple<bool, std::shared_ptr<packet_base>> handle_engine_io(std::string& in_data);

  boost::asio::awaitable<std::shared_ptr<packet_base>> async_event();

  void seed_message(const std::shared_ptr<packet_base>& in_message);
  // 取消消息队列等待
  void cancel_async_event();

 private:
  boost::asio::awaitable<void> impl_run();
  struct lock_type {
    sid_data* data_;
    explicit lock_type(sid_data* in_data) : data_{in_data} { ++data_->lock_count_; }
    ~lock_type() { --data_->lock_count_; }
  };
  friend class socket_io_websocket_core;
  sid_ctx* ctx_;
  const uuid sid_;
  std::atomic<chrono::sys_time_pos> last_time_;
  std::atomic_bool is_upgrade_to_websocket_;
  std::atomic_int lock_count_;
  std::atomic_bool close_;
  channel_type channel_;
  // 阻止消息接收
  std::atomic_bool block_message_;

  std::map<std::string, socket_io_core_ptr> socket_io_contexts_;
  boost::asio::cancellation_signal channel_signal_;
  std::shared_ptr<boost::asio::system_timer> timer_;

  struct block_message_guard {
    sid_data* data_;
    explicit block_message_guard(sid_data* in_data) : data_{in_data} { data_->block_message_ = true; }
    ~block_message_guard() { data_->block_message_ = false; }
  };
};

}  // namespace doodle::socket_io
