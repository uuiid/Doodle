//
// Created by TD on 2022/8/17.
//

#pragma once

#include <bitset>

#include <fmt/format.h>
#include <fmt/ostream.h>

namespace fmt{

template <std::size_t N>
struct formatter<::std::bitset<N>>: ::fmt::ostream_formatter {};
}
