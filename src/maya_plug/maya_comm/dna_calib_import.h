//
// Created by TD on 2024/1/11.
//

#pragma once

#include <maya_plug/main/maya_plug_fwd.h>
#include <maya_plug/maya_plug_fwd.h>

namespace doodle::maya_plug {
constexpr char dna_calib_import_name[]{"dna_calib_import"};
MSyntax dna_calib_import_syntax();
class dna_calib_import : public TemplateAction<dna_calib_import, dna_calib_import_name, dna_calib_import_syntax> {
  class impl;
  std::unique_ptr<impl> p_i;
  MStatus get_arg(const MArgList& in_arg);

 public:
  dna_calib_import();
  ~dna_calib_import() override;
  MStatus doIt(const MArgList& in_list) override;
};
}  // namespace doodle::maya_plug