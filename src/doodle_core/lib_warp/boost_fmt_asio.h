//
// Created by td_main on 2023/9/8.
//

#pragma once
#include <boost/asio.hpp>
#include <boost/beast.hpp>

#include <fmt/format.h>
#include <fmt/ostream.h>
namespace fmt {

template <typename Protocol, typename Executor>
struct formatter<boost::asio::basic_stream_socket<Protocol, Executor> > : formatter<std::string> {
  template <typename FormatContext>
  auto format(const boost::asio::basic_stream_socket<Protocol, Executor> &in_, FormatContext &ctx) const
      -> decltype(ctx.out()) {
    auto l_str = fmt::format(
        "{}|{}:{}", fmt::ptr(&in_), in_.remote_endpoint().address().to_string(), in_.remote_endpoint().port()
    );
    return formatter<std::string>::format(l_str, ctx);
  }
};
template <typename Protocol, typename Executor, typename RatePolicy>
struct formatter<boost::beast::basic_stream<Protocol, Executor, RatePolicy> > : formatter<std::string> {
  template <typename FormatContext>
  auto format(const boost::beast::basic_stream<Protocol, Executor, RatePolicy> &in_, FormatContext &ctx) const
      -> decltype(ctx.out()) {
    const auto &l_socket = in_.socket();
    std::string l_str    = fmt::format(
        "{}|{}:{}", fmt::ptr(&l_socket), l_socket.remote_endpoint().address().to_string(),
        l_socket.remote_endpoint().port()
    );
    return formatter<std::string>::format(l_str, ctx);
  }
};
}  // namespace fmt