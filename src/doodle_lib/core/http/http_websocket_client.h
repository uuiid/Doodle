//
// Created by TD on 24-7-25.
//

#pragma once
#include <doodle_lib/core/co_queue.h>
#include <doodle_lib/doodle_lib_fwd.h>

#include <boost/asio/awaitable.hpp>
#include <boost/url/url.hpp>

#include "http_route.h"
#include <tl/expected.hpp>

namespace doodle::http {

boost::asio::awaitable<std::shared_ptr<boost::beast::websocket::stream<boost::beast::tcp_stream>>>
make_websocket_stream(const boost::urls::url& in_url);

}  // namespace doodle::http