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

class DOODLE_CORE_EXPORT listener : public std::enable_shared_from_this<listener> {
 private:
  class impl;
  std::unique_ptr<impl> ptr;

 public:
  explicit listener(boost::asio::ip::tcp::endpoint in_end_point);
  void run();

 private:
  void do_accept();
  void on_accept(boost::beast::error_code ec, boost::asio::ip::tcp::socket socket);
};

}  // namespace doodle::http_server
