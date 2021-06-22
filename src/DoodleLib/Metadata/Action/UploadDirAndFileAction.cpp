//
// Created by TD on 2021/6/21.
//

#include "UploadDirAndFileAction.h"

#include <DoodleLib/Exception/Exception.h>
#include <Logger/Logger.h>
#include <Metadata/AssetsFile.h>
#include <Metadata/AssetsPath.h>
#include <rpc/RpcFileSystemClient.h>

doodle::UploadDirAndFileAction::UploadDirAndFileAction()
    : Action() {
  p_name = "上传多个路径";
}
doodle::UploadDirAndFileAction::UploadDirAndFileAction(std::any&& in_paths)
    : Action(std::move(in_paths)) {
  p_name = "上传多个路径";
}
std::string doodle::UploadDirAndFileAction::class_name() {
  return p_name;
}
void doodle::UploadDirAndFileAction::run(const doodle::MetadataPtr& in_data) {
  auto k_ch = CoreSet::getSet().getRpcFileSystemClient();
  if (!p_any.has_value()) {
    DOODLE_LOG_INFO("没有值")
    throw DoodleError{"没有值"};
  }
  if (p_any.type() != typeid(std::vector<FSys::path>)) {
    DOODLE_LOG_DEBUG("动作喂入参数无效")
    throw DoodleError{"动作喂入参数无效"};
  }
  auto k_path = std::any_cast<std::vector<FSys::path>>(p_any);

  auto k_ass_file = std::dynamic_pointer_cast<AssetsFile>(in_data);
  if (!k_ass_file) {
    DOODLE_LOG_DEBUG("无效的上传数据")
    throw DoodleError{"无效的上传数据"};
  }
  std::vector<AssetsPathPtr> k_list;
  for (auto& k_i : k_path) {
    auto k_ass_path = k_list.emplace_back(std::make_shared<AssetsPath>(k_i,k_ass_file));
    k_ch->Upload(k_ass_path->getLocalPath(),k_ass_path->getServerPath());
  }
  k_ass_file->setPathFile(k_list);
}
void doodle::UploadDirAndFileAction::operator()(const doodle::MetadataPtr& in_data) {
  run(in_data);
}
