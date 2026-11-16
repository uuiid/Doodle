//
// Created by TD on 2026/11/16.
//

#pragma once

#include <maya_plug/main/maya_plug_fwd.h>
#include <maya_plug/maya_plug_fwd.h>

namespace doodle::maya_plug {
constexpr char face_capture_import_name[]{"face_capture_import"};
MSyntax face_capture_import_syntax();
class face_capture_import
    : public TemplateAction<face_capture_import, face_capture_import_name, face_capture_import_syntax> {
  class impl;
  std::unique_ptr<impl> p_i;
  MStatus get_arg(const MArgList& in_arg);

 public:
  face_capture_import();
  ~face_capture_import() override;
  MStatus doIt(const MArgList& in_list) override;
};
}  // namespace doodle::maya_plug
