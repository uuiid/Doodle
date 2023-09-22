//
// Created by td_main on 2023/8/3.
//

#pragma once
#include <doodle_core/core/global_function.h>
#include <doodle_core/lib_warp/boost_fmt_asio.h>
#include <doodle_core/lib_warp/boost_fmt_error.h>

#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/render_farm/detail/basic_json_body.h>
#include <doodle_lib/render_farm/render_farm_fwd.h>

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/hana.hpp>
#include <boost/signals2.hpp>
#include <boost/url.hpp>

#include <memory>
#include <nlohmann/json.hpp>
namespace doodle::render_farm {

struct working_machine_session_data {
  explicit working_machine_session_data(boost::asio::ip::tcp::socket in_socket) : stream_(std::move(in_socket)) {}
  boost::beast::tcp_stream stream_;
  boost::beast::flat_buffer buffer_;
  boost::url url_;
  // copy delete
  working_machine_session_data(const working_machine_session_data&)                = delete;
  working_machine_session_data& operator=(const working_machine_session_data&)     = delete;
  // move
  working_machine_session_data(working_machine_session_data&&) noexcept            = default;
  working_machine_session_data& operator=(working_machine_session_data&&) noexcept = default;
};

namespace session {
struct request_parser_empty_body {
  std::unique_ptr<boost::beast::http::request_parser<boost::beast::http::empty_body>> request_parser_;
  request_parser_empty_body()
      : request_parser_(std::make_unique<boost::beast::http::request_parser<boost::beast::http::empty_body>>()) {}
  // copy delete
  request_parser_empty_body(const request_parser_empty_body&)                = delete;
  request_parser_empty_body& operator=(const request_parser_empty_body&)     = delete;
  // move
  request_parser_empty_body(request_parser_empty_body&&) noexcept            = default;
  request_parser_empty_body& operator=(request_parser_empty_body&&) noexcept = default;

  inline boost::beast::http::request_parser<boost::beast::http::empty_body>& operator*() { return *request_parser_; }
  inline boost::beast::http::request_parser<boost::beast::http::empty_body>* operator->() {
    return request_parser_.get();
  }
};

template <typename MsgBody>
struct async_read_body {
  std::unique_ptr<boost::beast::http::request_parser<MsgBody>> request_parser_;

  explicit async_read_body(request_parser_empty_body& in_request_parser_empty_body)
      : request_parser_(
            std::make_unique<boost::beast::http::request_parser<MsgBody>>(std::move(*in_request_parser_empty_body))
        ) {}
  explicit async_read_body(const entt::handle& in_handle)
      : request_parser_(std::make_unique<boost::beast::http::request_parser<MsgBody>>(
            std::move(*in_handle.get<request_parser_empty_body>())
        )) {}
  // copy delete
  async_read_body(const async_read_body&)                = delete;
  async_read_body& operator=(const async_read_body&)     = delete;
  // move
  async_read_body(async_read_body&&) noexcept            = default;
  async_read_body& operator=(async_read_body&&) noexcept = default;

  inline boost::beast::http::request_parser<boost::beast::http::string_body>& operator*() { return *request_parser_; }
  inline boost::beast::http::request_parser<boost::beast::http::string_body>* operator->() {
    return request_parser_.get();
  }
};

struct capture_url {
  std::map<std::string, std::string> capture_map_;
  explicit capture_url(std::map<std::string, std::string> in_map) : capture_map_(std::move(in_map)) {}

  template <typename T, std::enable_if_t<std::is_arithmetic_v<T>>* = nullptr>
  std::optional<T> get(const std::string& in_str) const {
    if (capture_map_.find(in_str) != capture_map_.end()) {
      return boost::lexical_cast<T>(capture_map_.at(in_str));
    }
    return {};
  }
  template <typename T, std::enable_if_t<!std::is_arithmetic_v<T>>* = nullptr>
  std::optional<T> get(const std::string& in_str) const {
    if (capture_map_.find(in_str) != capture_map_.end()) {
      return capture_map_.at(in_str);
    }
    return {};
  }
};

struct do_read {
  entt::handle handle_{};
  explicit do_read(entt::handle in_handle) : handle_(std::move(in_handle)) {}
  void run();
  void operator()(boost::system::error_code ec, std::size_t bytes_transferred);
};

template <typename MsgBody, typename CompletionHandler, typename ExecutorType>
struct do_read_msg_body : boost::beast::async_base<std::decay_t<CompletionHandler>, ExecutorType>,
                          boost::asio::coroutine {
  entt::handle handle_{};
  using async_read_body = session::async_read_body<MsgBody>;
  using msg_t           = boost::beast::http::request<MsgBody>;
  explicit do_read_msg_body(
      entt::handle in_handle, CompletionHandler&& in_handler, const ExecutorType& in_executor_type_1
  )
      : boost::beast::async_base<
            std::decay_t<CompletionHandler>,
            ExecutorType>{std::forward<CompletionHandler>(in_handler), in_executor_type_1},
        boost::asio::coroutine{},
        handle_(std::move(in_handle)) {}
  void run();
  void operator()(boost::system::error_code ec, std::size_t bytes_transferred);
};

struct do_close {
  entt::handle handle_{};
  boost::system::error_code ec{};
  logger_ptr logger_;
  explicit do_close(entt::handle in_handle) : handle_(std::move(in_handle)) {}

  void run();

  void operator()();
};

struct do_write {
  entt::handle handle_;
  bool keep_alive_{};
  boost::beast::http::message_generator message_generator_;
  explicit do_write(entt::handle in_handle, boost::beast::http::message_generator in_message_generator)
      : handle_(std::move(in_handle)), message_generator_(std::move(in_message_generator)) {}

  void run();
  static void send_error_code(
      entt::handle in_handle, boost::system::error_code ec,
      boost::beast::http::status in_status = boost::beast::http::status::bad_request
  );
  void operator()(boost::system::error_code ec, std::size_t bytes_transferred);
};

template <typename MsgBody, typename CompletionHandler, typename ExecutorType>
void do_read_msg_body<MsgBody, CompletionHandler, ExecutorType>::run() {
  if (handle_ && handle_.all_of<working_machine_session_data, request_parser_empty_body>()) {
    auto&& [l_data, l_body] = handle_.get<working_machine_session_data, request_parser_empty_body>();
    l_data.stream_.expires_after(30s);

    boost::beast::http::async_read(
        l_data.stream_, l_data.buffer_, *handle_.emplace_or_replace<async_read_body>(l_body), std::move(*this)
    );
  } else {
    boost::beast::error_code ec{};
    BOOST_BEAST_ASSIGN_EC(ec, error_enum::invalid_handle);
    log_error(fmt::format("无效的句柄"));
    this->complete(false, ec, handle_, msg_t{});
  }
}
template <typename MsgBody, typename CompletionHandler, typename ExecutorType>
void do_read_msg_body<MsgBody, CompletionHandler, ExecutorType>::operator()(
    boost::system::error_code ec, std::size_t bytes_transferred
) {
  if (!handle_) {
    BOOST_BEAST_ASSIGN_EC(ec, error_enum::invalid_handle);
    log_error(fmt::format("无效的句柄"));
    this->complete(false, ec, handle_, msg_t{});
    return;
  }
  auto l_logger = handle_.get<socket_logger>().logger_;
  if (ec) {
    if (ec != boost::beast::http::error::end_of_stream) {
      log_error(l_logger, fmt::format("on_write error: {} ", ec));
    } else {
      log_warn(l_logger, fmt::format("末端的流, 主动关闭 {} ", ec));
      do_close{handle_}.run();
    }
    this->complete(false, ec, handle_, msg_t{});
    return;
  }

  if (handle_.all_of<working_machine_session_data, async_read_body>()) {
    auto&& [l_data, l_body] = handle_.get<working_machine_session_data, async_read_body>();
    l_data.stream_.expires_after(30s);
    this->complete(true, ec, handle_, l_body->release());
    return;
  }
  BOOST_BEAST_ASSIGN_EC(ec, error_enum::component_missing_error);
  log_error(l_logger, fmt::format("缺失必要组件, working_machine_session_data, async_read_body"));
  this->complete(false, ec, handle_, msg_t{});
}
}  // namespace session

/**
 * @brief 会话类 用于处理客户端的请求  一个会话对应一个客户端
 */
class working_machine_session {
 public:
  explicit working_machine_session(boost::asio::ip::tcp::socket in_socket, http_route_ptr in_route_ptr_)
      : ptr_(std::make_shared<data_type>(std::move(in_socket))) {
    ptr_->route_ptr_ = std::move(in_route_ptr_);
  }
  ~working_machine_session() = default;

  void run();

  template <typename Error_Type>
  void send_error(
      const Error_Type& in_error, boost::beast::http::status in_status = boost::beast::http::status::bad_request
  ) {
    boost::beast::http::response<detail::basic_json_body> l_response{in_status, 11};
    l_response.body() = {{"state", boost::diagnostic_information(in_error)}, {"id", -1}};
    l_response.prepare_payload();
    l_response.keep_alive(false);
    send_response(boost::beast::http::message_generator{std::move(l_response)});
  };
  template <typename Error_Type>
  void send_error_code(
      const Error_Type& in_error, boost::beast::http::status in_status = boost::beast::http::status::bad_request
  ) {
    boost::beast::http::response<detail::basic_json_body> l_response{in_status, 11};
    l_response.body() = {{"state", in_error.message()}, {"id", -1}};
    l_response.prepare_payload();
    l_response.keep_alive(false);
    send_response(boost::beast::http::message_generator{std::move(l_response)});
  };

 private:
  void do_read();
  /**
   * @brief 解析请求,并返回响应
   * @param ec 错误码
   * @param bytes_transferred  读取的字节数
   */
  void on_parser(boost::system::error_code ec, std::size_t bytes_transferred);
  void on_write(bool keep_alive, boost::system::error_code ec, std::size_t bytes_transferred);

  using request_parser_type     = boost::beast::http::request_parser<boost::beast::http::empty_body>;
  using request_parser_type_ptr = std::shared_ptr<request_parser_type>;

  struct data_type {
    data_type(boost::asio::ip::tcp::socket in_socket) : stream_(std::move(in_socket)) {}
    boost::beast::tcp_stream stream_;

    boost::beast::flat_buffer buffer_;
    request_parser_type_ptr request_parser_;
    boost::url url_;
    boost::signals2::scoped_connection connection_;
    http_route_ptr route_ptr_;
    logger_ptr logger_;
  };
  std::shared_ptr<data_type> ptr_;

 public:
  void send_response(boost::beast::http::message_generator&& in_message_generator);
  void do_close();
  [[nodiscard("")]] inline boost::beast::http::request_parser<boost::beast::http::empty_body>& request_parser() {
    return *ptr_->request_parser_;
  };
  [[nodiscard("")]] inline boost::beast::tcp_stream& stream() { return ptr_->stream_; }
  [[nodiscard("")]] inline const boost::beast::tcp_stream& stream() const { return ptr_->stream_; }
  [[nodiscard("")]] inline boost::beast::tcp_stream stream_release() { return std::move(ptr_->stream_); }
  // buffer
  [[nodiscard("")]] inline boost::beast::flat_buffer& buffer() { return ptr_->buffer_; }
  [[nodiscard("")]] inline const boost::beast::flat_buffer& buffer() const { return ptr_->buffer_; }
  // url
  [[nodiscard("")]] inline boost::url& url() { return ptr_->url_; }
  [[nodiscard("")]] inline const boost::url& url() const { return ptr_->url_; }
};

}  // namespace doodle::render_farm
