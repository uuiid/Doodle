//
// Created by TD on 2024/2/20.
//

#pragma once

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/url.hpp>
namespace doodle::http {
class http_session_data {
 private:
  void do_read(boost::system::error_code ec, std::size_t bytes_transferred);

 public:
  explicit http_session_data(boost::asio::ip::tcp::socket in_socket)
      : stream_(std::make_unique<boost::beast::tcp_stream>(std::move(in_socket))),
        buffer_{},
        url_{},
        request_parser_{} {}
  std::unique_ptr<boost::beast::tcp_stream> stream_;
  boost::beast::flat_buffer buffer_;
  boost::url url_;
  std::unique_ptr<boost::beast::http::request_parser<boost::beast::http::empty_body>> request_parser_;

  inline boost::beast::tcp_stream& operator*() const { return *stream_; }
  inline boost::beast::tcp_stream* operator->() const { return stream_.get(); }

  // copy delete
  http_session_data(const http_session_data&)                = delete;
  http_session_data& operator=(const http_session_data&)     = delete;
  // move
  http_session_data(http_session_data&&) noexcept            = default;
  http_session_data& operator=(http_session_data&&) noexcept = default;

  void rend_head();

  void do_close();

  template <typename name>
  auto send_error(boost::beast::http::status in_status, const std::string& in_reason);
};

namespace session {
template <typename MsgBody>
struct async_read_body;

template <typename MsgBody>
struct async_read_body {
  std::unique_ptr<boost::beast::http::request_parser<MsgBody>> request_parser_;

  template <typename T>
  explicit async_read_body(async_read_body<T>& in_request_parser_empty_body)
    requires(std::is_same_v<T, boost::beast::http::empty_body>);

  explicit async_read_body(const entt::handle& in_handle);

  // copy delete
  async_read_body(const async_read_body&)                = delete;
  async_read_body& operator=(const async_read_body&)     = delete;
  // move
  async_read_body(async_read_body&&) noexcept            = default;
  async_read_body& operator=(async_read_body&&) noexcept = default;

  inline boost::beast::http::request_parser<MsgBody>& operator*() const { return *request_parser_; }
  inline boost::beast::http::request_parser<MsgBody>* operator->() const { return request_parser_.get(); }
};

template <>
struct async_read_body<boost::beast::http::empty_body> {
  std::unique_ptr<boost::beast::http::request_parser<boost::beast::http::empty_body>> request_parser_;

  async_read_body()
      : request_parser_(std::make_unique<boost::beast::http::request_parser<boost::beast::http::empty_body>>()) {}
  // copy delete
  async_read_body(const async_read_body&)                = delete;
  async_read_body& operator=(const async_read_body&)     = delete;
  // move
  async_read_body(async_read_body&&) noexcept            = default;
  async_read_body& operator=(async_read_body&&) noexcept = default;

  inline boost::beast::http::request_parser<boost::beast::http::empty_body>& operator*() const {
    return *request_parser_;
  }
  inline boost::beast::http::request_parser<boost::beast::http::empty_body>* operator->() const {
    return request_parser_.get();
  }
};
using request_parser_empty_body = async_read_body<boost::beast::http::empty_body>;

template <typename MsgBody>
async_read_body<MsgBody>::async_read_body(const entt::handle& in_handle)
    : request_parser_(std::make_unique<boost::beast::http::request_parser<MsgBody>>(
          std::move(*in_handle.get<async_read_body<boost::beast::http::empty_body>>())
      )) {}

template <typename MsgBody>
template <typename T>
async_read_body<MsgBody>::async_read_body(async_read_body<T>& in_request_parser_empty_body)
  requires(std::is_same_v<T, boost::beast::http::empty_body>)
    : request_parser_(
          std::make_unique<boost::beast::http::request_parser<MsgBody>>(std::move(*in_request_parser_empty_body))
      ) {}

}  // namespace session

}  // namespace doodle::http