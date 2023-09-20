//
// Created by td_main on 2023/9/14.
//

#pragma once
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/lib_warp/boost_fmt_asio.h>
#include <doodle_core/lib_warp/boost_fmt_error.h>
#include <doodle_core/logger/logger.h>

#include <doodle_lib/render_farm/render_farm_fwd.h>

#include <boost/beast.hpp>

#include <nlohmann/json.hpp>
namespace doodle::render_farm {
using error_type_id   = entt::tag<"error_type_id"_hs>;
using reply_type_id   = entt::tag<"reply_type_id"_hs>;
using request_type_id = entt::tag<"request_type_id"_hs>;

class websocket;
struct websocket_data {
  explicit websocket_data(boost::beast::tcp_stream in_stream) : stream_(std::move(in_stream)) {}

  boost::beast::websocket::stream<boost::beast::tcp_stream> stream_;
  boost::beast::flat_buffer buffer_{};

  std::queue<std::string> write_queue{};
  bool write_flag_{};

  std::queue<std::string> read_queue{};
  bool read_flag_{};

  std::map<std::int64_t, std::function<void(boost::system::error_code, const nlohmann::json&)>> call_map_{};
  std::int64_t id_{};

  std::weak_ptr<websocket> websocket_ptr_{};

  std::shared_ptr<boost::asio::signal_set> signal_set_{};
};

namespace details {
struct websocket_tmp_data {
  using resolver_t = boost::asio::ip::tcp::resolver;
  using result_t   = resolver_t::results_type;
  template <typename T>
  explicit websocket_tmp_data(T&& in_exe) : resolver_(std::forward<T>(in_exe)) {}
  resolver_t resolver_;
  result_t resolver_results_;
  std::string path_;
  std::string server_address_;
  std::uint16_t server_port_;
};
}  // namespace details
/**
 * @brief websocket 两端对等,不区分客户端和服务器
 */
class websocket : public std::enable_shared_from_this<websocket> {
 private:
  entt::handle data_{};
  void do_read();

  void run_fun();
  void do_write();
  void do_destroy();
  void do_connect();
  void do_handshake();
  void fail_call(boost::system::error_code in_code);
  void fail_call(boost::system::error_code in_code, std::int64_t in_id);

 public:
  websocket() = default;
  explicit websocket(entt::handle in_data) : data_(std::move(in_data)) {}
  ~websocket()                               = default;

  // copy
  websocket(const websocket&)                = delete;
  websocket& operator=(const websocket&)     = delete;
  // move
  websocket(websocket&&) noexcept            = default;
  websocket& operator=(websocket&&) noexcept = default;

  void close();
  /// 服务器回复使用
  void run(const boost::beast::http::request<boost::beast::http::string_body>& in_message);
  /// 客户端连接使用
  void run(std::string server_address, std::string path, std::uint16_t server_port = doodle_config::http_port);

  void send_error_code(const boost::system::error_code& in_code, std::uint64_t in_id);

  template <typename Call_T>
  auto async_call(nlohmann::json& in_json, Call_T&& in_call_) {
    auto& l_data           = data_.get<websocket_data>();
    auto l_id              = ++l_data.id_;
    in_json["id"]          = l_id;
    in_json["jsonrpc"]     = "2.0";
    l_data.call_map_[l_id] = std::forward<Call_T>(in_call_);
    l_data.write_queue.emplace(in_json.dump());
    do_write();
  }
};

}  // namespace doodle::render_farm