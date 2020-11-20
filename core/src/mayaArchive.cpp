#include "mayaArchive.h"
#include "shotfilesqlinfo.h"

#include "Logger.h"
#include <boost/filesystem.hpp>
CORE_NAMESPACE_S
mayaArchive::mayaArchive(fileSqlInfoPtr shot_data)
    : p_info_ptr_(std::move(shot_data)) {}

void mayaArchive::insertDB() {

  p_info_ptr_->setFileList(p_Path);
  p_info_ptr_->setInfoP("maya动画文件");
  p_info_ptr_->insert();
}
void mayaArchive::_generateFilePath() {

  if (!p_soureFile.empty())
    p_Path.push_back(
        p_info_ptr_->generatePath(
            "Scenefiles",
            boost::filesystem::extension(p_soureFile[0])
        ).generic_string());
  else if (!p_info_ptr_->getFileList().empty()) {
    for (auto &&item :p_info_ptr_->getFileList()) {
      p_Path.push_back(item.generic_string());
    }
  }
}

CORE_NAMESPACE_E
