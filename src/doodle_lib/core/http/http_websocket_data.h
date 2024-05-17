//
// Created by TD on 2024/2/20.
//

#pragma once

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/lib_warp/boost_fmt_error.h>
#include <doodle_core/logger/logger.h>

#include <doodle_lib/core/http/socket_logger.h>

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/url.hpp>

#include <entt/entt.hpp>
#include <nlohmann/json.hpp>
namespace doodle::http {

enum class http_websocket_data_fun { ping, set_state, set_task, logger };
NLOHMANN_JSON_SERIALIZE_ENUM(
    http_websocket_data_fun, {{http_websocket_data_fun::ping, "ping"},
                              {http_websocket_data_fun::set_state, "set_state"},
                              {http_websocket_data_fun::set_task, "set_task"},
                              {http_websocket_data_fun::logger, "logger"}}
);
class http_session_data;
using http_session_data_ptr = std::shared_ptr<http_session_data>;
class http_websocket_data : public std::enable_shared_from_this<http_websocket_data> {
 private:
  using resolver_t       = boost::asio::ip::tcp::resolver;
  using resolver_ptr     = std::unique_ptr<resolver_t>;
  using result_t         = resolver_t::results_type;
  using websocket_stream = boost::beast::websocket::stream<boost::beast::tcp_stream>;

  resolver_ptr resolver_;
  result_t resolver_results_;

 public:
  std::unique_ptr<websocket_stream> stream_;
  boost::beast::flat_buffer buffer_{};  // (Must persist between reads)
  // read queue
  std::queue<std::string> read_queue_;
  bool is_reading_ = false;

  // write queue
  std::queue<std::string> write_queue_;
  bool is_writing_ = false;

  // logger
  logger_ptr logger_{};

  // user data
  std::shared_ptr<void> user_data_;

  // 读写守卫
  class read_guard_t {
   public:
    explicit read_guard_t(http_websocket_data* in_ptr) : ptr_(in_ptr) { ptr_->is_reading_ = true; }
    ~read_guard_t() { ptr_->is_reading_ = false; }
    read_guard_t(const read_guard_t&)                = delete;
    read_guard_t& operator=(const read_guard_t&)     = delete;
    read_guard_t(read_guard_t&&) noexcept            = default;
    read_guard_t& operator=(read_guard_t&&) noexcept = default;

   private:
    http_websocket_data* ptr_;
  };
  class write_guard_t {
   public:
    explicit write_guard_t(http_websocket_data* in_ptr) : ptr_(in_ptr) { ptr_->is_writing_ = true; }
    ~write_guard_t() { ptr_->is_writing_ = false; }
    write_guard_t(const write_guard_t&)                = delete;
    write_guard_t& operator=(const write_guard_t&)     = delete;
    write_guard_t(write_guard_t&&) noexcept            = default;
    write_guard_t& operator=(write_guard_t&&) noexcept = default;

   private:
    http_websocket_data* ptr_;
  };

 private:
  template <typename CompletionHandler, typename ExecutorType>
  struct connect_op : boost::beast::async_base<std::decay_t<CompletionHandler>, ExecutorType>, boost::asio::coroutine {
    http_websocket_data* ptr_;
    std::string server_address_;
    std::string url_path_;
    explicit connect_op(
        std::string in_server_address, std::uint16_t in_server_port_, std::string in_url_path,
        http_websocket_data* in_handle, CompletionHandler&& in_handler, const ExecutorType& in_executor_type_1
    )
        : boost::beast::async_base<std::decay_t<CompletionHandler>, ExecutorType>(
              std::move(in_handler), in_executor_type_1
          ),
          boost::asio::coroutine{},
          ptr_(std::move(in_handle)),
          server_address_(std::move(in_server_address)),
          url_path_(std::move(in_url_path)) {
      ptr_->resolver_->async_resolve(server_address_, std::to_string(in_server_port_), std::move(*this));
    }

    ~connect_op()                                = default;
    // move
    connect_op(connect_op&&) noexcept            = default;
    connect_op& operator=(connect_op&&) noexcept = default;
    // copy
    connect_op(const connect_op&)                = delete;
    connect_op& operator=(const connect_op&)     = delete;
    // on_resolve
    void operator()(boost::system::error_code ec, boost::asio::ip::tcp::resolver::results_type in_results) {
      auto l_handle = entt::handle{*g_reg(), entt::to_entity(*g_reg(), *ptr_)};
      auto l_logger = l_handle.get<socket_logger>().logger_;
      if (ec) {
        l_logger->log(log_loc(), level::err, "async_resolve error: {} ", ec);
        this->complete(false, ec);
        return;
      }
      ptr_->stream_ = std::make_unique<websocket_stream>(boost::asio::get_associated_executor(
          ptr_->resolver_->get_executor(), boost::asio::make_strand(g_io_context())
      ));
      boost::beast::get_lowest_layer(*ptr_->stream_).async_connect(in_results, std::move(*this));
    }
    // on_connect
    void operator()(boost::system::error_code ec, boost::asio::ip::tcp::resolver::results_type::endpoint_type) {
      auto l_handle = entt::handle{*g_reg(), entt::to_entity(*g_reg(), *ptr_)};
      auto l_logger = l_handle.get<socket_logger>().logger_;
      if (ec) {
        l_logger->log(log_loc(), level::err, "async_connect error: {} ", ec);
        this->complete(false, ec);
        return;
      }

      boost::beast::get_lowest_layer(*ptr_->stream_).expires_never();
      ptr_->stream_->set_option(boost::beast::websocket::stream_base::timeout::suggested(boost::beast::role_type::client
      ));
      ptr_->stream_->set_option(
          boost::beast::websocket::stream_base::decorator([](boost::beast::websocket::request_type& req) {
            req.set(boost::beast::http::field::user_agent, std::string(BOOST_BEAST_VERSION_STRING) + " doodle");
          })
      );
      ptr_->stream_->async_handshake(server_address_, url_path_, std::move(*this));
    }
    // on_handshake
    void operator()(boost::system::error_code ec) {
      auto l_handle = entt::handle{*g_reg(), entt::to_entity(*g_reg(), *ptr_)};
      auto l_logger = l_handle.get<socket_logger>().logger_;
      if (ec) {
        l_logger->log(log_loc(), level::err, "async_handshake error: {} ", ec);
        this->complete(false, ec);
        return;
      }
      ptr_->do_read();
      this->complete(false, ec);
    }
  };

 public:
  explicit http_websocket_data(boost::beast::tcp_stream in_stream)
      : stream_(std::make_unique<websocket_stream>(std::move(in_stream))) {}
  http_websocket_data() = default;

  template <typename CompletionHandler>
  auto async_connect(
      std::string server_address, std::string path, std::uint16_t server_port, CompletionHandler&& in_handler
  ) {
    resolver_ = std::make_unique<resolver_t>(
        boost::asio::get_associated_executor(in_handler, boost::asio::make_strand(g_io_context()))
    );
    return boost::asio::async_initiate<CompletionHandler, void(boost::system::error_code)>(
        [](auto&& handler, auto* ptr, std::string server_address, std::uint16_t server_port, std::string path) {
          connect_op{
              server_address,
              server_port,
              path,
              ptr,
              std::forward<decltype(handler)>(handler),
              boost::asio::get_associated_executor(handler, boost::asio::make_strand(g_io_context()))
          };
        },
        in_handler, this, std::move(server_address), server_port, std::move(path)
    );
  }

  void run(const http_session_data_ptr& in_data);
  void do_read();
  void do_write();

  void run_fun();

  boost::signals2::signal<void(const nlohmann::json&, const std::shared_ptr<http_websocket_data>&)> on_message;

  // 不一定有回复, 所以不需要回调
  void seed(const nlohmann::json& in_json);
  void do_close();
};

class http_websocket_data_manager {
  // 共享锁
  std::shared_mutex mutex_;
  // 数据
  std::vector<std::weak_ptr<http_websocket_data>> data_list_;

 public:
  http_websocket_data_manager()  = default;
  ~http_websocket_data_manager() = default;

  void push_back(const std::shared_ptr<http_websocket_data>& in_data);

  std::vector<std::shared_ptr<http_websocket_data>> get_list();
};
http_websocket_data_manager& g_websocket_data_manager();

}  // namespace doodle::http