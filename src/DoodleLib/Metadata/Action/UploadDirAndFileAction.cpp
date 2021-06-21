//
// Created by TD on 2021/6/21.
//

#include "UploadDirAndFileAction.h"
doodle::UploadDirAndFileAction::UploadDirAndFileAction()
    : Action() {
  p_name = "上传多个路径";
}
doodle::UploadDirAndFileAction::UploadDirAndFileAction(std::any&& in_paths)
    : Action(std::move(in_paths)) {
  p_name = "上传多个路径";
} std::string doodle::UploadDirAndFileAction::class_name() {
  return p_name;
}
void doodle::UploadDirAndFileAction::run(const doodle::MetadataPtr& in_data) {
}
void doodle::UploadDirAndFileAction::operator()(const doodle::MetadataPtr& in_data) {
}
