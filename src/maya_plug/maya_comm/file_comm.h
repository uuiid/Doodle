//
// Created by TD on 2021/12/15.
//

#pragma once
#include <main/maya_plug_fwd.h>
namespace doodle::maya_plug {
namespace {
constexpr char comm_file_save_name[] = "doodle_comm_file_save";
}
namespace details {
MSyntax comm_file_save_syntax();
}
class comm_file_save
    : public doodle::TemplateAction<comm_file_save, comm_file_save_name, details::comm_file_save_syntax> {
 public:
  MStatus doIt(const MArgList& in_arg) override;
};

}  // namespace doodle::maya_plug
