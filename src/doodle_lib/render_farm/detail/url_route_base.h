//
// Created by td_main on 2023/8/9.
//

#pragma once

#include <boost/beast.hpp>
#include <boost/url.hpp>
namespace doodle::render_farm {
namespace detail {
std::pair<boost::urls::segments_ref::iterator, boost::urls::segments_ref::iterator> chick_url(
    boost::urls::segments_ref in_segments_ref
);

}  // namespace detail
}  // namespace doodle::render_farm