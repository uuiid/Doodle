//
// Created by TD on 2022/7/1.
//

#pragma once

#include <maya_plug/main/maya_plug_fwd.h>
namespace doodle::maya_plug {

namespace dem_bones_add_weight_ns {
constexpr char name[] = "doodle_comm_dem_bones_weiget";
MSyntax syntax();
}  // namespace dem_bones_add_weight_ns

class dem_bones_add_weight : public doodle::TemplateAction<
                                 dem_bones_add_weight, dem_bones_add_weight_ns::name, dem_bones_add_weight_ns::syntax> {
  class impl;
  std::unique_ptr<impl> p_i;

  void get_arg(const MArgList& in_arg);
  void add_weight();

 public:
  dem_bones_add_weight();
  ~dem_bones_add_weight() override;
  MStatus doIt(const MArgList& in_arg) override;
};

}  // namespace doodle::maya_plug
