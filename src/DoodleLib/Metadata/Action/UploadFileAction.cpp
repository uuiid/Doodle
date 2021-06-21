//
// Created by TD on 2021/6/17.
//

#include "UploadFileAction.h"
namespace doodle {
UploadFileAction::UploadFileAction()
    : Action() {
  p_name = "上传文件"
}
UploadFileAction::UploadFileAction(std::any&& in_any)
    : Action(std::move(in_any)) {
  p_name = "上传文件"
}
std::string UploadFileAction::class_name() {
  return p_name;
}
void UploadFileAction::run(const MetadataPtr& in_data) {
}
void UploadFileAction::operator()(const MetadataPtr& in_data) {
}

}  // namespace doodle
