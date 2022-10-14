//
// Created by TD on 2022/10/14.
//

#include "export_files.h"

namespace doodle {
namespace maya_plug {

namespace export_files_ns {}

class export_files::impl {
 public:
};

export_files::export_files() : ptr(std::make_unique<impl>()) {}

void export_files::run() {}

void export_files::get_arg(const MArgList& in_arg_list) {}

MStatus export_files::doIt(const MArgList& in_arg_list) {
  get_arg(in_arg_list);
  run();
  return {MStatus::MStatusCode::kSuccess};
}
}  // namespace maya_plug
}  // namespace doodle