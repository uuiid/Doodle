//
// Created by TD on 25-2-12.
//

#pragma once
#include <doodle_core/doodle_core_fwd.h>

#include <boost/signals2/signal.hpp>

namespace doodle::socket_io {
class sid_ctx;
class sid_data;
struct socket_io_packet;
using socket_io_packet_ptr = std::shared_ptr<socket_io_packet>;
class socket_io_websocket_core;
using socket_io_websocket_core_ptr  = std::shared_ptr<socket_io_websocket_core>;
using socket_io_websocket_core_wptr = std::weak_ptr<socket_io_websocket_core>;
using socket_io_sid_data_ptr        = std::shared_ptr<sid_data>;
using socket_io_sid_data_wptr       = std::weak_ptr<sid_data>;
/// socket 连接
class socket_io_core : public std::enable_shared_from_this<socket_io_core> {
 public:
  using signal_type = boost::signals2::signal<void(const std::variant<nlohmann::json, std::vector<std::string>>&)>;
  using signal_ptr  = std::shared_ptr<signal_type>;
  using slot_type   = signal_type::slot_type;

 private:
  /// 这回 sid 和 sid_data 不同, 时socket.io 多路复用的sid, 而不是连接的sid
  uuid sid_;
  sid_ctx* ctx_;
  /// 当前负责发送消息的 sid_data
  socket_io_sid_data_wptr sid_data_;
  std::string namespace_;
  /// 当前接收的事件
  socket_io_packet_ptr current_packet_;
  /// 发送事件的信号
  std::map<std::string, signal_ptr> signal_map_;
  std::optional<boost::signals2::scoped_connection> on_emit_scoped_connection_{};
  std::optional<boost::signals2::scoped_connection> on_message_scoped_connection_{};

  struct current_packet_guard {
    socket_io_core* self{};
    explicit current_packet_guard(const socket_io_packet_ptr& in_data, socket_io_core* in_core) : self(in_core) {
      self->current_packet_ = in_data;
    }
    ~current_packet_guard() { self->current_packet_.reset(); }
  };

  void on_impl(const socket_io_packet_ptr&);

 public:
  /**
   *
   * @param in_ctx 总上下文
   * @param in_namespace 连接使用的名称空间
   * @param in_json 初次连接时的负载
   */
  explicit socket_io_core(sid_ctx* in_ctx, const socket_io_sid_data_ptr& in_sid_data);
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
  boost::asio::awaitable<void> set_namespace(const std::string& in_namespace, const nlohmann::json& in_json);

  nlohmann::json auth_{};

  void emit(const std::string& in_event, const nlohmann::json& in_data) const;
  void emit(const std::string& in_event, const std::vector<std::string>& in_data) const;

  /**
   * 这个只能在同步的回调里面调用
   *只能在 on_message solt 槽中调用
   * @param in_data 回调数据
   *
   */
  void ask(const nlohmann::json& in_data) const;
  void ask(const std::vector<std::string>& in_data) const;

  template <typename Json_Call, typename String_Call = decltype([](const std::vector<std::string>&) {})>
    requires std::is_invocable_v<Json_Call, nlohmann::json>
  auto on_message(
      const std::string& in_event_name, Json_Call&& in_json_solt,
      String_Call&& in_string_solt = [](const std::vector<std::string>&) {}
  ) {
    if (!signal_map_[in_event_name]) signal_map_[in_event_name] = std::make_shared<signal_type>();
    auto l_overloaded =
        std::make_shared<overloaded<Json_Call, String_Call>>(std::move(in_json_solt), std::move(in_string_solt));
    return signal_map_[in_event_name]->connect(
        slot_type{[l_overloaded](const std::variant<nlohmann::json, std::vector<std::string>>& in_data) {
          std::visit(*l_overloaded, in_data);
        }}.track_foreign(shared_from_this())
    );
  }
};
}  // namespace doodle::socket_io
