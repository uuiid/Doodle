//
// Created by TD on 2022/8/17.
//

#pragma once

#include <boost/icl/split_interval_set.hpp>
#include <boost/icl/discrete_interval.hpp>
#include <boost/icl/interval_map.hpp>
#include <boost/icl/interval_set.hpp>
#include <fmt/format.h>
#include <fmt/ostream.h>

namespace fmt {
template <typename Type>
struct formatter<Type,
                 std::enable_if_t<boost::icl::is_interval_map<Type>::value, void>>
    : ostream_formatter {};
template <typename Type>
struct formatter<Type,
                 std::enable_if_t<boost::icl::is_interval_set<Type>::value, void>>
    : ostream_formatter {};
}  // namespace fmt
