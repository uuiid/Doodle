//
// Created by TD on 2022/3/4.
//

#pragma once

#include <maya_plug_fwd.h>
#include <maya_plug/data/maya_poly_info.h>

namespace doodle::maya_plug {

class find_duplicate_poly {
 public:
  std::vector<std::pair<MObject, MObject>> operator()(const MObjectArray& in_array);
};

}  // namespace doodle::maya_plug
