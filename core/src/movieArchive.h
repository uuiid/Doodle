#pragma once

#include "core_global.h"
#include "fileArchive.h"
CORE_NAMESPACE_S
class CORE_EXPORT movieArchive : public fileArchive {
 public:
  explicit movieArchive(shotInfoPtr &shot_info_ptr);
  bool makeMovie(const QString &imageFolder);
  bool update(const stringList &filelist) override;
 protected:
  void insertDB() override;
  void _generateFilePath() override;

 private:
  shotInfoPtr p_info_ptr_;
};

CORE_NAMESPACE_E