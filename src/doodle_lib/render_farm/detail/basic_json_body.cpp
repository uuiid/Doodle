//
// Created by td_main on 2023/8/10.
//

#include "basic_json_body.h"

namespace doodle::render_farm::detail {
std::pair<boost::urls::segments_ref::iterator, boost::urls::segments_ref::iterator> chick_url(
    boost::urls::segments_ref in_segments_ref
) {
  auto l_begin = in_segments_ref.begin();
  auto l_end   = in_segments_ref.end();
  if (l_begin == l_end) {
    throw_exception(doodle_error{"url error"});
  }

  if (*l_begin != "v1") {
    throw_exception(doodle_error{"url version error"});
  }
  ++l_begin;
  if (l_begin == l_end) {
    throw_exception(doodle_error{"url not found render_farm"});
  }
  if (*l_begin != "render_farm") {
    throw_exception(doodle_error{"url not found render_farm"});
  }
  ++l_begin;
  if (l_begin == l_end) {
    throw_exception(doodle_error{"url not found action"});
  }
  return {l_begin, l_end};
}

}  // namespace doodle::render_farm::detail