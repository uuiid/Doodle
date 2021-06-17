//
// Created by TD on 2021/6/17.
//

#include "DragFilesFactory.h"

#include <FileWarp/ImageSequence.h>
#include <FileWarp/MayaFile.h>
#include <FileWarp/Ue4Project.h>
#include <core/Ue4Setting.h>

#include <Metadata/Action/UploadDirAction.h>
#include <Metadata/Action/UploadDirAction.h>
namespace doodle {
DragFilesFactory::DragFilesFactory(std::vector<FSys::path> in_paths)
    : p_paths(std::move(in_paths)),
      p_has_action(false){
}
ActionPtr DragFilesFactory::get_action() {
  return ActionPtr();
}
ActionPtr DragFilesFactory::operator()() {
  return ActionPtr();
}
bool DragFilesFactory::has_action() {
  return p_has_action;
}
void DragFilesFactory::runChick() {
  if (p_paths.size() == 1) {
    auto k_path = p_paths.at(0);

    if (FSys::is_directory(k_path)) {
    } else {
      if (Ue4Project::is_ue4_file(k_path)) {
      } else if (MayaFile::is_maya_file(k_path)) {
      } else {
      }
    }

  } else {
  }
}

};  // namespace doodle
