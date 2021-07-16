//
// Created by TD on 2021/7/16.
//

#include "actn_maya_export.h"

#include <DoodleLib/FileWarp/MayaFile.h>
#include <Gui/action/actn_up_paths.h>
namespace doodle {

actn_maya_export::actn_maya_export()
    : p_up_paths(std::make_shared<actn_up_paths>()) {
  p_name = "导出maya fbx 并上传文件";
}
bool actn_maya_export::is_async() {
  return true;
}
long_term_ptr actn_maya_export::run(const MetadataPtr& in_data, const MetadataPtr& in_parent) {
  return p_term;
}
bool actn_maya_export::is_accept(const action::arg_path& in_any) {
  return MayaFile::is_maya_file(in_any.date);
}
}  // namespace doodle
