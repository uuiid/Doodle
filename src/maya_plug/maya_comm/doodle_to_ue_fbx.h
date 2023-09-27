//
// Created by td_main on 2023/9/26.
//
#pragma once
#include <maya_plug/main/maya_plug_fwd.h>

#include "maya/MApiNamespace.h"
namespace doodle {
namespace maya_plug {
namespace doodle_to_ue_fbx_ns {
constexpr char doodle_to_ue_fbx[]{"doodle_to_ue_fbx"};
}  // namespace doodle_to_ue_fbx_ns
MSyntax doodle_to_ue_fbx_syntax();

class doodle_to_ue_fbx
    : public TemplateAction<doodle_to_ue_fbx, doodle_to_ue_fbx_ns::doodle_to_ue_fbx, doodle_to_ue_fbx_syntax> {
  class impl_data;
  std::unique_ptr<impl_data> p_i;

  void write_fbx();

 public:
  doodle_to_ue_fbx();
  ~doodle_to_ue_fbx() override;
  MStatus doIt(const MArgList& in_list) override;
};

}  // namespace maya_plug
}  // namespace doodle
