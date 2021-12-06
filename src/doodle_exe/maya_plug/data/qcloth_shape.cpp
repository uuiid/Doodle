//
// Created by TD on 2021/12/6.
//

#include "qcloth_shape.h"

#include <maya_plug/command/reference_file.h>
namespace doodle::maya_plug {
qcloth_shape::qcloth_shape() = default;

qcloth_shape::qcloth_shape(const entt::handle& in_ref_file)
    : p_ref_file(in_ref_file) {
  chick_component<reference_file>(p_ref_file);
}

}  // namespace doodle::maya_plug
