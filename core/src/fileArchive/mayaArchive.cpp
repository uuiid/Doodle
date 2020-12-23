/*
 * @Author: your name
 * @Date: 2020-09-27 14:33:50
 * @LastEditTime: 2020-12-01 13:58:59
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\core\src\fileArchive\mayaArchive.cpp
 */
#include "mayaArchive.h"
#include "src/shots/shotfilesqlinfo.h"

#include "Logger.h"
#include <boost/filesystem.hpp>
CORE_NAMESPACE_S
mayaArchive::mayaArchive(fileSqlInfoPtr shot_data)
    : p_info_ptr_(std::move(shot_data)) {}

bool mayaArchive::useUpdataCheck() const {
  return true;
}

bool mayaArchive::updataCheck() const {
  return true;
}

bool mayaArchive::useDowndataCheck() const {
  return true;
}

bool mayaArchive::downdataCheck() const {
  return true;
}

void mayaArchive::setUseCustomPath(const dpathPtr& custom_path) {
}

void mayaArchive::insertDB() {
  p_info_ptr_->setFileList(p_ServerPath);
  if (p_info_ptr_->getInfoP().empty()) {
    p_info_ptr_->setInfoP("maya动画文件");
  }
  if (p_info_ptr_->isInsert())
    p_info_ptr_->updateSQL();
  else
    p_info_ptr_->insert();
}
void mayaArchive::_generateFilePath() {
  if (!p_soureFile.empty()) {
    if (isServerzinsideDir(p_soureFile.front())) {
      p_ServerPath.push_back(p_soureFile.front());
      return;
    }
    p_ServerPath.push_back(
        p_info_ptr_->generatePath(
            "Scenefiles",
            boost::filesystem::extension(p_soureFile[0])));
  } else if (!p_info_ptr_->getFileList().empty()) {
    for (auto&& item : p_info_ptr_->getFileList()) {
      p_ServerPath.push_back(item);
    }
  }
}

CORE_NAMESPACE_E
