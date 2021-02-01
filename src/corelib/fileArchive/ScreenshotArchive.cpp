#include "ScreenshotArchive.h"

#include <loggerlib/Logger.h>
#include <corelib/filesystem/FileSystem.h>

#include <corelib/fileDBInfo/filesqlinfo.h>
#include <corelib/Exception/Exception.h>
DOODLE_NAMESPACE_S

ScreenshotArchive::ScreenshotArchive(fileSqlInfoPtr info_ptr)
    : fileArchive(),
      p_info_ptr_(std::move(info_ptr)) {
}

std::unique_ptr<std::fstream> ScreenshotArchive::loadImage() {
  down();
  if (!boost::filesystem::exists(p_cacheFilePath.front())) {
    DOODLE_LOG_WARN("没有文件： " << p_ServerPath.front().generic_string())
    throw not_file_error(p_ServerPath.front().generic_string());
  }

  auto p = std::make_unique<boost::filesystem::fstream>(
      p_cacheFilePath.front(), std::ios_base::out);
  if (!p->is_open()) {
    p->open(p_cacheFilePath.front(), std::ios_base::out);
  }
  return p;
}

void ScreenshotArchive::insertDB() {
  if (!DfileSyntem::get().exists(p_ServerPath.front())) {
    DOODLE_LOG_WARN("没有文件： " << p_ServerPath.front().generic_string())
    throw not_file_error(p_ServerPath.front().generic_string());
  }

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