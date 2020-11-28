#pragma once

#include "core_global.h"
#include "src/fileArchive/fileArchive.h"
CORE_NAMESPACE_S
class CORE_API movieArchive : public fileArchive {
 public:
  explicit movieArchive(fileSqlInfoPtr shot_info_ptr);
  bool makeMovie(const dpath &imageFolder);
  bool convertMovie(const dpath &moviePath);
  bool update(const dpathList &filelist) override;
//  bool update(const std::vector<QString> &filelist);
 protected:
  void insertDB() override;
  void _generateFilePath() override;

  virtual void setInfoAttr() = 0;

  fileSqlInfoPtr p_info_ptr_;
};

CORE_NAMESPACE_E