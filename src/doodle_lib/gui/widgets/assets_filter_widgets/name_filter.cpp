//
// Created by TD on 2022/5/7.
//

#include "name_filter.h"

#include <doodle_core/metadata/assets_file.h>
namespace doodle {
namespace gui {
bool name_filter::operator()(const entt::handle& in) const {
  if (in.any_of<assets_file>()) {
    return boost::algorithm::contains(in.get<assets_file>().p_user, name_);
  } else
    return false;
}
}  // namespace gui
}  // namespace doodle
