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
  bool is_handshake_{};
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
  void fail_call(boost::system::error_code in_code);
  void fail_call(boost::system::error_code in_code, std::int64_t in_id);

  template <typename CompletionHandler, typename ExecutorType>
  struct connect_op : boost::beast::async_base<std::decay_t<CompletionHandler>, ExecutorType>, boost::asio::coroutine {
    entt::handle data_;
    explicit connect_op(entt::handle in_handle, CompletionHandler&& in_handler, const ExecutorType& in_executor_type_1)
        : boost::beast::async_base<std::decay_t<CompletionHandler>, ExecutorType>(
              std::move(in_handler), in_executor_type_1
          ),
          boost::asio::coroutine{},
          data_(std::move(in_handle)) {
      auto& l_data = data_.get<details::websocket_tmp_data>();
      l_data.resolver_.async_resolve(l_data.server_address_, std::to_string(l_data.server_port_), std::move(*this));
    }

    ~connect_op()                                = default;
    // move
    connect_op(connect_op&&) noexcept            = default;
    connect_op& operator=(connect_op&&) noexcept = default;
    // copy
    connect_op(const connect_op&)                = delete;
    connect_op& operator=(const connect_op&)     = delete;
    // on_resolve
    void operator()(boost::system::error_code ec, boost::asio::ip::tcp::resolver::results_type results) {
      auto l_logger = data_.get<socket_logger>().logger_;
      if (ec) {
        log_error(l_logger, fmt::format("async_resolve error: {} ", ec));
        this->complete(false, ec);
        return;
      }
      if (!data_ || !data_.all_of<details::websocket_tmp_data>()) return;
      data_.get<details::websocket_tmp_data>().resolver_results_ = std::move(results);
      boost::beast::get_lowest_layer(data_.get<websocket_data>().stream_)
          .async_connect(data_.get<details::websocket_tmp_data>().resolver_results_, std::move(*this));
    }
    // on_connect
    void operator()(boost::system::error_code ec, boost::asio::ip::tcp::resolver::results_type::endpoint_type) {
      auto l_logger = data_.get<socket_logger>().logger_;
      if (ec) {
        log_error(l_logger, fmt::format("async_connect error: {} ", ec));
        this->complete(false, ec);
        return;
      }
      if (!data_ || !data_.all_of<details::websocket_tmp_data>()) return;

      auto&& [l_data, l_path] = data_.get<websocket_data, details::websocket_tmp_data>();
      boost::beast::get_lowest_layer(l_data.stream_).expires_never();
      l_data.stream_.set_option(boost::beast::websocket::stream_base::timeout::suggested(boost::beast::role_type::client
      ));
      l_data.stream_.set_option(
          boost::beast::websocket::stream_base::decorator([](boost::beast::websocket::request_type& req) {
            req.set(boost::beast::http::field::user_agent, std::string(BOOST_BEAST_VERSION_STRING) + " doodle");
          })
      );
      auto l_host = l_path.server_address_ + ":" + std::to_string(l_path.server_port_);
      data_.get<websocket_data>().stream_.async_handshake(
          data_.get<details::websocket_tmp_data>().server_address_, data_.get<details::websocket_tmp_data>().path_,
          std::move(*this)
      );
    }
    // on_handshake
    void operator()(boost::system::error_code ec) {
      auto l_logger = data_.get<socket_logger>().logger_;
      if (ec) {
        log_error(l_logger, fmt::format("async_handshake error: {} ", ec));
        this->complete(false, ec);
        return;
      }
      if (!data_ || !data_.all_of<details::websocket_tmp_data>()) return;
      data_.get<websocket_data>().is_handshake_ = true;
      this->complete(false, ec);
    }
  };

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
  template <typename CompletionHandler>
  auto async_connect(
      std::string server_address, std::string path, std::uint16_t server_port, CompletionHandler&& in_handler
  ) {
    auto l_strand = boost::asio::make_strand(g_io_context());
    if (!data_.all_of<socket_logger>()) data_.emplace<socket_logger>();
    if (!data_.all_of<websocket_data>()) data_.emplace<websocket_data>(boost::beast::tcp_stream{l_strand});
    auto& l_data           = data_.get_or_emplace<details::websocket_tmp_data>(l_strand);
    l_data.server_address_ = std::move(server_address);
    l_data.path_           = std::move(path);
    l_data.server_port_    = server_port;
    return boost::asio::async_initiate<CompletionHandler, void(boost::system::error_code)>(
        [l_data_ = data_, l_strand](auto&& in_handler) {
          connect_op<CompletionHandler, std::decay_t<decltype(l_strand)>>{
              l_data_, std::forward<decltype(in_handler)>(in_handler), l_strand};
        },
        in_handler
    );
  }

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