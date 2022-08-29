//
// Created by TD on 2022/8/26.
//

#pragma once

#include <doodle_core/doodle_core_fwd.h>

#include <boost/beast.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/strand.hpp>

namespace doodle::http_server {

class DOODLE_CORE_EXPORT session : public std::enable_shared_from_this<session> {
 public:
  //    namespace beast = boost::beast;          // from <boost/beast.hpp>
  //  namespace http  = beast::http;           // from <boost/beast/http.hpp>
  //  namespace net   = boost::asio;           // from <boost/asio.hpp>
  using tcp = boost::asio::ip::tcp;  // from <boost/asio/ip/tcp.hpp>
 private:
  //  beast::tcp_stream stream_;
  //  beast::flat_buffer buffer_;
  //  std::shared_ptr<std::string const> doc_root_;
  //  http::request<http::string_body> req_;
  //  std::shared_ptr<void> res_;
  class impl;
  std::unique_ptr<impl> ptr;

 public:
  explicit session(
      tcp::socket&& socket
  );
  virtual ~session();

  void run();
  void do_read();
  void on_read(
      boost::beast::error_code ec,
      std::size_t bytes_transferred
  );
  void on_write(
      bool close,
      boost::beast::error_code ec,
      std::size_t bytes_transferred
  );
  void do_close();
};

}  // namespace doodle::http_server
