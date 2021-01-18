#include "ScreenshotArchive.h"

#include <Logger.h>
#include <src/fileDBInfo/filesqlinfo.h>

DOODLE_NAMESPACE_S

ScreenshotArchive::ScreenshotArchive(fileSqlInfoPtr info_ptr)
    : fileArchive(),
      p_info_ptr_(std::move(info_ptr)) {
}

void ScreenshotArchive::insertDB() {
  p_info_ptr_->setFileList(p_ServerPath);
  if (p_info_ptr_->getInfoP().empty()) {
    p_info_ptr_->setInfoP("截图文件");
  }
  if (p_info_ptr_->isInsert())
    p_info_ptr_->updateSQL();
  else
    p_info_ptr_->insert();
}

void ScreenshotArchive::imp_generateFilePath() {
  if (!p_soureFile.empty()) {
    p_ServerPath.push_back(
        p_info_ptr_->generatePath(
            "doodle", ".png"));
  } else if (!p_info_ptr_->getFileList().empty()) {
    for (auto&& item : p_info_ptr_->getFileList()) {
      p_ServerPath.push_back(item);
    }
  } else {
    throw std::runtime_error("无法生成路径");
    DOODLE_LOG_WARN("无法生成路径");
  }
}

DOODLE_NAMESPACE_E