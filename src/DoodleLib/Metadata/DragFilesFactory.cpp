//
// Created by TD on 2021/6/17.
//

#include "DragFilesFactory.h"

#include <FileWarp/ImageSequence.h>
#include <FileWarp/MayaFile.h>
#include <FileWarp/Ue4Project.h>
#include <Metadata/Action/Action.h>
#include <Metadata/Action/UploadDirAction.h>
#include <Metadata/Action/UploadDirAndFileAction.h>
#include <Metadata/Action/UploadFileAction.h>
#include <core/Ue4Setting.h>
namespace doodle {
DragFilesFactory::DragFilesFactory(std::vector<FSys::path> in_paths)
    : p_paths(std::move(in_paths)),
      p_has_action(false) {
}
ActionPtr DragFilesFactory::get_action() {
  return ActionPtr();
}
ActionPtr DragFilesFactory::operator()() {
  return ActionPtr();
}
bool DragFilesFactory::has_action() {
  return !p_action.empty();
}
void DragFilesFactory::runChick() {
  if (p_paths.size() == 1) {
    auto k_path = p_paths.at(0);

    if (FSys::is_directory(k_path)) {
      p_action.emplace_back(std::make_shared<UploadDirAction>(
          std::make_any<FSys::path>(k_path)));
    } else {
      if (Ue4Project::is_ue4_file(k_path)) {
        std::vector<FSys::path> k_list{};
        k_list.emplace_back(k_path);
        k_list.emplace_back(k_path.parent_path() / Ue4Project::Content);
        p_action.emplace_back(
            std::make_shared<UploadDirAndFileAction>(
                std::make_any<std::vector<FSys::path>>(
                    k_list)));
      } else {
        p_action.emplace_back(
            std::make_shared<UploadFileAction>(
                std::make_any<FSys::path>(k_path)));
      }
    }
  } else {

  }
}

};  // namespace doodle
