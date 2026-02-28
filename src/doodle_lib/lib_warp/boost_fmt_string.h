//
// Created by td_main on 2023/9/15.
//

#pragma once

#include <boost/static_string.hpp>

#include <fmt/format.h>
#include <fmt/ostream.h>

namespace fmt {
template <typename Char_T>
struct formatter<::boost::basic_string_view<Char_T>> : ostream_formatter {};
}  // namespace fmt
