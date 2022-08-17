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
// template <
//     typename DomainT,
//     typename CodomainT,
//     class Traits,
//     ICL_COMPARE Compare,
//     ICL_COMBINE Combine,
//     ICL_SECTION Section,
//     ICL_INTERVAL(ICL_COMPARE) Interval,
//     ICL_ALLOC Alloc>
// struct formatter<::boost::icl::interval_map<
//     DomainT,
//     CodomainT,
//     Traits,
//     Compare,
//     Combine,
//     Section,
//     Interval,
//     Alloc>> {};

// template <typename DomainT,
//           ICL_COMPARE Compare,
//           typename Interval,
//           ICL_ALLOC Alloc>
// struct formatter<::boost::icl::interval_set<DomainT,
//                                             Compare,
//                                             Interval,
//                                             Alloc>>
//     : ostream_formatter {};


template <>
struct formatter<::boost::icl::interval_bounds>
    : ostream_formatter {};

 template <typename Type>
 struct formatter<Type,
                  std::enable_if_t<::boost::icl::is_discrete_interval<Type>::value, char>>
     : ostream_formatter {};

//template <typename Type>
//struct formatter<Type,
//                 std::enable_if_t<::boost::icl::is_interval_map<Type>::value, char>>
//    : formatter<fmt::string_view> {
//  template <typename FormatContext>
//  auto format(const Type &in_interval_map, FormatContext &in_ctx) const
//      -> decltype(in_ctx.out()) {
//    fmt::format_to(in_ctx.out(), "{{");
//    for (auto &&i_interval : in_interval_map) {
//      fmt::format_to(in_ctx.out(), "({}->{})", i_interval.first, i_interval.second);
//    }
//    fmt::format_to(in_ctx.out(), "}}");
//    return in_ctx.out();
//  }
//};

template <typename Type>
struct formatter<Type,
                 std::enable_if_t<::boost::icl::is_interval_set<Type>::value, char>>
    : ostream_formatter {};
}  // namespace fmt
