//
// Created by td_main on 2023/9/14.
//

#pragma once

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/logger/logger.h>

#include <boost/beast.hpp>
namespace doodle::render_farm {
using error_type_id   = entt::tag<"error_type_id"_hs>;
using reply_type_id   = entt::tag<"reply_type_id"_hs>;
using request_type_id = entt::tag<"request_type_id"_hs>;

class websocket {
 private:
  struct impl {
    explicit impl(boost::asio::ip::tcp::socket in_stream) : stream_(std::move(in_stream)) {}
    boost::beast::websocket::stream<boost::asio::ip::tcp::socket> stream_;
    boost::beast::flat_buffer buffer_{};
    logger_ptr logger_{};

    std::queue<std::string> write_queue{};
    bool write_flag_{};

    std::queue<std::string> read_queue{};
    bool read_flag_{};
  };

  std::unique_ptr<impl> impl_ptr_{};
  void do_read();
  void make_ptr();

  void run_fun();
  void do_write();

 public:
  websocket() = default;
  explicit websocket(boost::asio::ip::tcp::socket in_stream) : impl_ptr_(std::make_unique<impl>(std::move(in_stream))) {
    make_ptr();
  }
  ~websocket()                               = default;

  // copy
  websocket(const websocket&)                = delete;
  websocket& operator=(const websocket&)     = delete;
  // move
  websocket(websocket&&) noexcept            = default;
  websocket& operator=(websocket&&) noexcept = default;

  void run(const boost::beast::http::request<boost::beast::http::string_body>& in_message);

  void send_error_code(const boost::system::error_code& in_code, std::uint64_t in_id);
};

}  // namespace doodle::render_farm