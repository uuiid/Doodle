#include "mayaArchive.h"
#include "shotfilesqlinfo.h"

#include "Logger.h"

CORE_NAMESPACE_S
mayaArchive::mayaArchive(shotInfoPtr &shot_data)
    : p_info_ptr_(shot_data) {}

void mayaArchive::insertDB() {

  p_info_ptr_->setFileList(p_Path);
  p_info_ptr_->insert();
}
void mayaArchive::_generateFilePath() {

  if (!p_soureFile.empty())
    p_Path.push_back( p_info_ptr_->generatePath("Scenefiles",
                                                QFileInfo(p_soureFile[0]).suffix()));
  else if (!p_info_ptr_->getFileList().isEmpty()) {
    for(auto &&item :p_info_ptr_->getFileList()){
      p_Path.push_back(item.filePath());
    }
  }
}


CORE_NAMESPACE_E
