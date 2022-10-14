//
// Created by TD on 2022/10/14.
//

#pragma once

#include <maya_plug/main/maya_plug_fwd.h>

namespace doodle::maya_plug {
namespace export_files_ns {
constexpr char g_name[] = "doodle_export_files";
MSyntax g_syntax();
}  // namespace export_files_ns
class export_files : public doodle::TemplateAction<export_files, export_files_ns::g_name, export_files_ns::g_syntax> {
 private:
  class impl;
  std::unique_ptr<impl> ptr;

  void run();
  void get_arg(const MArgList& in_arg_list);

  void save_file();

 public:
  export_files();
  MStatus doIt(const MArgList& in_arg_list) override;
};

}  // namespace doodle::maya_plug
