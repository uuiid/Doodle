//
// Created by TD on 2021/6/17.
//

#include "upload_file_action.h"

#include <DoodleLib/Exception/Exception.h>
#include <Logger/Logger.h>
#include <Metadata/AssetsFile.h>
#include <Metadata/AssetsPath.h>
#include <rpc/RpcFileSystemClient.h>


namespace doodle {
upload_file_action::upload_file_action()
    : action() {
  p_name = "上传文件";
}
upload_file_action::upload_file_action(std::any&& in_any)
    : action(std::move(in_any)) {
  p_name = "上传文件";
}
std::string upload_file_action::class_name() {
  return p_name;
}
void upload_file_action::run(const MetadataPtr& in_data) {
  auto k_ch = CoreSet::getSet().getRpcFileSystemClient();
  if (!p_any.has_value()) {
    DOODLE_LOG_INFO("没有值")
    throw DoodleError{"没有值"};
  }
  if (p_any.type() != typeid(FSys::path)) {
    DOODLE_LOG_DEBUG("动作喂入参数无效")
    throw DoodleError{"动作喂入参数无效"};
  }
  auto k_path = std::any_cast<FSys::path>(p_any);

  auto k_ass_file = std::dynamic_pointer_cast<AssetsFile>(in_data);
  if (!k_ass_file) {
    DOODLE_LOG_DEBUG("无效的上传数据")
    throw DoodleError{"无效的上传数据"};
  }
  auto k_ass_path = std::make_shared<AssetsPath>(k_path,in_data);
  k_ass_file->setPathFile({k_ass_path});

  k_ch->Upload(k_ass_path->getLocalPath(),k_ass_path->getServerPath());
}
void upload_file_action::operator()(const MetadataPtr& in_data) {
  run(in_data);
}

}  // namespace doodle
