//
// Created by td_main on 2023/9/8.
//

#pragma once

#include <boost/url.hpp>

#include <fmt/format.h>
#include <fmt/ostream.h>

namespace fmt {
template <>
struct formatter<::boost::url> : ostream_formatter {};
}  // namespace fmt