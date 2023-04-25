//
// Created by TD on 2022/6/30.
//

#pragma once

#include <maya_plug/main/maya_plug_fwd.h>

namespace doodle::maya_plug {

namespace dem_bones_comm_ns {
constexpr char name[] = "doodle_comm_dem_bones";
MSyntax syntax();
}  // namespace dem_bones_comm_ns

class dem_bones_comm
    : public doodle::TemplateAction<dem_bones_comm, dem_bones_comm_ns::name, dem_bones_comm_ns::syntax> {
  class impl;
  std::unique_ptr<impl> p_i;
  void get_arg(const MArgList& in_arg);

  void create_joins();
  void create_anm_curve();
  void set_result();

 public:
  dem_bones_comm();
  ~dem_bones_comm() override;
  MStatus doIt(const MArgList& in_arg) override;
};

class dem_bones_add_wieget;

}  // namespace doodle::maya_plug
