#pragma once

#include "core_global.h"

#include <src/fileArchive/fileArchive.h>

DOODLE_NAMESPACE_S

class ScreenshotArchive : public fileArchive {
 public:
  ScreenshotArchive();

 protected:
  void insertDB() override;
  void imp_generateFilePath() override;

 private:
};

DOODLE_NAMESPACE_E