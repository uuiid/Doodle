#pragma once

#include <corelib/core_global.h>
#include <corelib/fileArchive/fileArchive.h>
DOODLE_NAMESPACE_S
class CORE_API imageArchive : public fileArchive {
 public:
  //explicit取消掉隐式转换
  explicit imageArchive(fileSqlInfoPtr f_ptr);
  virtual bool update(const dpathList &filelist) override;
  using fileArchive::update;

 protected:
  void insertDB() override;
  void imp_generateFilePath() override;
};

DOODLE_NAMESPACE_E