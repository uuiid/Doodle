//
// Created by td_main on 2023/8/3.
//
#pragma once
#include "doodle_core/configure/static_value.h"
#include "doodle_core/core/global_function.h"

#include "boost/asio.hpp"
#include "boost/beast.hpp"
#include <boost/asio/cancellation_signal.hpp>

#include <cstdint>
#include <memory>
#include <utility>
namespace doodle::http {
class http_route;
using http_route_ptr = std::shared_ptr<http_route>;

namespace detail {
struct http_listener_cancellation_slot {
  boost::asio::cancellation_signal signal_;
  boost::asio::cancellation_slot slot() { return signal_.slot(); }
};
boost::asio::awaitable<void> run_http_listener(
    boost::asio::io_context& in_io_context, http_route_ptr in_route_ptr,
    std::uint16_t in_port = doodle_config::http_port
);
}  // namespace detail

void run_http_listener(
    boost::asio::io_context& in_io_context, http_route_ptr in_route_ptr,
    std::uint16_t in_port = doodle_config::http_port
);

}  // namespace doodle::http
