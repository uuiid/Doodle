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
struct formatter<boost::asio::basic_stream_socket<Protocol, Executor>> : formatter<std::string> {
  template <typename FormatContext>
  auto format(const boost::asio::basic_stream_socket<Protocol, Executor> &in_, FormatContext &ctx) const
      -> decltype(ctx.out()) {
    const auto l_is_open   = in_.is_open();
    const auto l_end_point = l_is_open ? in_.remote_endpoint() : boost::asio::ip::tcp::endpoint{};
    auto l_str =
        fmt::to_string(fmt::ptr(&in_)) +
        (l_is_open ? fmt::format("|{}:{}", l_end_point.address().to_string(), l_end_point.port()) : std::string{});
    return formatter<std::string>::format(l_str, ctx);
  }
};
template <typename Protocol, typename Executor, typename RatePolicy>
struct formatter<boost::beast::basic_stream<Protocol, Executor, RatePolicy>> : formatter<std::string> {
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

namespace fmt {
template <typename Char_T>
struct formatter<::boost::beast::basic_string_view<Char_T>> : ostream_formatter {};
}  // namespace fmt