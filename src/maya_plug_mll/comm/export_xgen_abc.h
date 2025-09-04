//
// Created by TD on 2025/9/3.
//

#pragma once

#include <maya_plug/maya_plug_fwd.h>
namespace doodle::maya_plug {
constexpr char xgen_abc_export_name[]{R"(doodle_xgen_abc_export)"};
MSyntax xgen_abc_export_syntax();
class xgen_abc_export : public TemplateAction<xgen_abc_export, xgen_abc_export_name, xgen_abc_export_syntax> {
 public:
  xgen_abc_export();
  ~xgen_abc_export();
  MStatus doIt(const MArgList &args) override;
  [[maybe_unused]] MStatus undoIt() override;
  [[maybe_unused]] MStatus redoIt() override;

 private:
  struct impl;
  std::unique_ptr<impl> p_i;
  void parse_args(const MArgList &args);
};
}  // namespace doodle::maya_plug