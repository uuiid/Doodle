//
// Created by TD on 2022/10/13.
//

#include "null_facet.h"

#include <maya_plug/data/maya_create_movie.h>

namespace doodle {
namespace maya_plug {
const std::string& null_facet::name() const noexcept {
  static std::string l_i{"null_facet"};
  return l_i;
}
void null_facet::operator()() {}
void null_facet::deconstruction() { work_lock.reset(); }
null_facet::null_facet() { g_reg()->ctx().emplace<image_to_move>() = std::make_shared<detail::maya_create_movie>(); }
}  // namespace maya_plug
}  // namespace doodle