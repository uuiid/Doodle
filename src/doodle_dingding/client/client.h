//
// Created by TD on 2022/9/7.
//

#pragma once

#include <doodle_dingding/doodle_dingding_fwd.h>

#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/system.hpp>
#include <boost/url/urls.hpp>

namespace doodle::dingding {
/**
 * @brief 一个特别基础的https客户端
 * 使用方法 run("www.example.com","443","/")
 */
class DOODLE_DINGDING_API client
    : public std::enable_shared_from_this<client> {
 private:
  constexpr static const std::string_view dingding_host{"https://oapi.dingtalk.com"};

  class impl;
  std::unique_ptr<impl> ptr;

  void on_resolve(
      boost::system::error_code ec,
      const boost::asio::ip::tcp::resolver::results_type& results
  );

  void on_connect(
      boost::system::error_code ec,
      const boost::asio::ip::tcp::resolver::results_type::endpoint_type&
  );

  void on_handshake(boost::system::error_code ec);
  void on_write(
      boost::system::error_code ec,
      std::size_t bytes_transferred
  );
  void on_read(
      boost::system::error_code ec,
      std::size_t bytes_transferred
  );
  void on_shutdown(boost::system::error_code ec);

 public:
  explicit client(
      const boost::asio::any_io_executor& in_executor,
      boost::asio::ssl::context& in_ssl_context
  );

  void run(
      const std::string& in_host,
      const std::int32_t& in_port,
      const std::string& in_target
  );
  void run(
      const std::string& in_host,
      const std::string& in_port,
      const std::string& in_target
  );
  void run(
      const std::string& in_host,
      const std::string& in_target
  );
  void run(
      boost::url& in_url
  );

  std::string gettoken();

  virtual ~client() noexcept;
};

}  // namespace doodle::dingding
