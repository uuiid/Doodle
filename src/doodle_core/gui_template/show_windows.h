//
// Created by TD on 2022/9/21.
//

#pragma once

#include <doodle_core/doodle_core_fwd.h>
#include <boost/type_erasure/any.hpp>
#include <boost/type_erasure/member.hpp>

namespace doodle::gui::detail {

BOOST_TYPE_ERASURE_MEMBER(tick)
BOOST_TYPE_ERASURE_MEMBER(title)

/**
 * 当 tick 返回 true 时, 会将其在定时器中弹出并销毁
 */
using windows_tick = boost::type_erasure::any<
    boost::mpl::vector<
        boost::type_erasure::typeid_<>,
        boost::type_erasure::relaxed,
        boost::type_erasure::copy_constructible<>,
        has_tick<bool()>>>;

using windows_render = boost::type_erasure::any<
    boost::mpl::vector<
        boost::type_erasure::typeid_<>,
        boost::type_erasure::relaxed,
        boost::type_erasure::copy_constructible<>,
        has_tick<bool()>,
        has_title<const std::string&() const>>>;

}  // namespace doodle::gui::detail
