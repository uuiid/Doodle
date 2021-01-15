#pragma once

#include "core_global.h"

#include <src/fileArchive/fileArchive.h>

DOODLE_NAMESPACE_S

class ScreenshotArchive : public fileArchive {
 public:
  ScreenshotArchive(fileSqlInfoPtr info_ptr);

 protected:
  void insertDB() override;
  void imp_generateFilePath() override;

 private:
  fileSqlInfoPtr p_info_ptr_;
};

DOODLE_NAMESPACE_E