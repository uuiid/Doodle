//
// Created by TD on 25-3-24.
//

#pragma once
#include <doodle_core/core/app_base.h>
#include <doodle_core/metadata/image_size.h>
#include <maya/MTime.h>
namespace doodle::maya_plug {
class export_rig_facet final {
  MTime anim_begin_time_{};
  // 输出结果路径
  FSys::path out_path_file_{};
 public:
  export_rig_facet()  = default;
  ~export_rig_facet() = default;
  bool post(const nlohmann::json& in_argh);
};
}  // namespace doodle::maya_plug
