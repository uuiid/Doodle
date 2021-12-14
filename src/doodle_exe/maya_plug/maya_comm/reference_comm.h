//
// Created by TD on 2021/12/13.
//

#pragma once
#include <maya_plug/maya_plug_fwd.h>

namespace doodle::maya_plug {
namespace {
constexpr char create_ref_file_command_name[] = "doodle_create_ref_file";
constexpr char ref_file_load_command_name[]   = "doodle_ref_file_load";
constexpr char ref_file_sim_command_name[]    = "doodle_ref_file_sim";
constexpr char ref_file_export_command_name[] = "doodle_ref_file_export";
}  // namespace
MSyntax create_ref_syntax();
MSyntax ref_file_sim_syntax();
MSyntax ref_file_export_syntax();
class create_ref_file_command : public TemplateAction<
                                    create_ref_file_command,
                                    create_ref_file_command_name,
                                    create_ref_syntax> {
 public:
  MStatus doIt(const MArgList&) override;
};

class ref_file_load_command : public TemplateAction<
                                  ref_file_load_command,
                                  ref_file_load_command_name> {
 public:
  MStatus doIt(const MArgList&) override;
};
class ref_file_sim_command : public TemplateAction<
                                 ref_file_sim_command,
                                 ref_file_sim_command_name,
                                 ref_file_sim_syntax> {
 public:
  MStatus doIt(const MArgList&) override;
};
class ref_file_export_command : public TemplateAction<
                                    ref_file_export_command,
                                    ref_file_export_command_name,
                                    ref_file_export_syntax> {
 public:
  enum export_type : std::uint32_t {
    abc = 0,
    fbx = 1
  };
  MStatus doIt(const MArgList&) override;
};
}  // namespace doodle::maya_plug
