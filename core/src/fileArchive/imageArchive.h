#pragma once

#include <core_global.h>
#include <src/fileArchive/fileArchive.h>
CORE_NAMESPACE_S
class CORE_API imageArchive : public fileArchive {
 public:
  //explicit取消掉隐式转换
  explicit imageArchive(fileSqlInfoPtr f_ptr);
  virtual bool update(const dpathList &filelist) override;
  using fileArchive::update;

 protected:
  void insertDB() override;
  void _generateFilePath() override;
};

CORE_NAMESPACE_E