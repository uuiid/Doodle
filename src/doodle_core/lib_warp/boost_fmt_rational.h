//
// Created by TD on 2022/8/24.
//

#pragma once

#include <fmt/format.h>
#include <fmt/ostream.h>
#include <boost/rational.hpp>

namespace fmt {
template <typename Type>
struct formatter<::boost::rational<Type>>
    : ostream_formatter {};
}  // namespace fmt
