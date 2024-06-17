//
// Created by td_main on 2023/9/15.
//

#pragma once
#include <boost/beast.hpp>

#include <fmt/format.h>
#include <fmt/ostream.h>

namespace fmt {
template <>
struct formatter<::boost::beast::http::verb> : ostream_formatter {};
}  // namespace fmt