//
// Created by td_main on 2023/6/7.
//
#pragma once

#include "main/maya_plug_fwd.h"
#include <maya_plug/main/maya_plug_fwd.h>

#include "maya/MApiNamespace.h"

namespace doodle {
namespace maya_plug {
namespace ref_file_export_ns {
constexpr char ref_file_export[]{"ref_file_export"};
}  // namespace ref_file_export_ns
MSyntax ref_file_export_syntax();
class ref_file_export
    : public TemplateAction<ref_file_export, ref_file_export_ns::ref_file_export, ref_file_export_syntax> {
 public:
  MStatus doIt(const MArgList& in_list) override;
};

}  // namespace maya_plug
}  // namespace doodle
