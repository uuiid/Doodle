//
// Created by TD on 2022/9/8.
//

#pragma once

#include <fmt/format.h>
#include <fmt/ostream.h>
#include <boost/beast.hpp>

namespace fmt {

template <bool isRequest, class Body, class Fields>
struct formatter<::boost::beast::http::message<isRequest, Body, Fields>>
    : ostream_formatter {};
}  // namespace fmt
