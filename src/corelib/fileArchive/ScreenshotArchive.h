#pragma once

#include <corelib/core_global.h>

#include <corelib/fileArchive/fileArchive.h>

DOODLE_NAMESPACE_S

class CORE_API ScreenshotArchive : public fileArchive {
 public:
  ScreenshotArchive(fileSqlInfoPtr info_ptr);

  std::unique_ptr<std::fstream> loadImage();

 protected:
  void insertDB() override;
  void imp_generateFilePath() override;

 private:
  fileSqlInfoPtr p_info_ptr_;
};

DOODLE_NAMESPACE_E