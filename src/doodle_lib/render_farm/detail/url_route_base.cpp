//
// Created by td_main on 2023/8/9.
//

#include "url_route_base.h"

#include <doodle_core/exception/exception.h>
namespace doodle::render_farm::detail {
void url_route_base::chick_v1_render_farm() {
  auto l_begin = segments_ref_.begin();
  auto l_end   = segments_ref_.end();

  if (l_begin == l_end) {
    throw_exception(doodle_error{"url is empty"});
  }

  if ((*l_begin) != "v1") {
    throw_exception(doodle_error{"url version is not v1"});
  }

  ++l_begin;
  if (l_begin == l_end) {
    throw_exception(doodle_error{"url not found render_farm"});
  }

  if ((*l_begin) != "render_farm") {
    throw_exception(doodle_error{"url not found render_farm"});
  }

  ++l_begin;
  if (l_begin == l_end) {
    throw_exception(doodle_error{"url not found id"});
  }
}

}  // namespace doodle::render_farm::detail
