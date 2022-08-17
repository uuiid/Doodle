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
template <

    typename DomainT,
    typename CodomainT,
    class Traits,
    ICL_COMPARE Compare,
    ICL_COMBINE Combine,
    ICL_SECTION Section,
    ICL_INTERVAL(ICL_COMPARE) Interval,
    ICL_ALLOC Alloc>
struct formatter<::boost::icl::interval_map<
    DomainT,
    CodomainT,
    Traits,
    Compare,
    Combine,
    Section,
    Interval,
    Alloc>>
    : ostream_formatter {};

template <typename DomainT,
          ICL_COMPARE Compare,
          typename Interval,
          ICL_ALLOC Alloc>
struct formatter<::boost::icl::interval_set<DomainT,
                                            Compare,
                                            Interval,
                                            Alloc>>
    : ostream_formatter {};
}  // namespace fmt
