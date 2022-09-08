//
// Created by TD on 2022/9/8.
//
#define BOOST_TEST_MODULE dingding
#include <boost/test/included/unit_test.hpp>
#include <doodle_dingding/client/client.h>
#include <boost/asio/ssl.hpp>
#include <boost/asio.hpp>
using namespace doodle;

BOOST_AUTO_TEST_CASE(client_base_tset) {
  boost::asio::io_context l_io_context{};
  /// \brief SSL 上下文是必需的，并持有证书
  boost::asio::ssl::context l_context{
      boost::asio::ssl::context::sslv23};
  /// 验证远程服务器的证书
  l_context.set_verify_mode(boost::asio::ssl::verify_peer);
  l_context.set_options(
      boost::asio::ssl::context::default_workarounds
      | boost::asio::ssl::context::no_sslv2
      | boost::asio::ssl::context::no_sslv3
  );
  l_context.set_default_verify_paths();
  using namespace std::literals;

  std::make_shared<dingding::client>(boost::asio::make_strand(l_io_context), l_context)
      ->run("www.baidu.com"s, "/"s);
  l_io_context.run();
}
