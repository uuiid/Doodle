#pragma once

#include "core_global.h"
#include "fileArchive.h"
CORE_NAMESPACE_S
class CORE_EXPORT movieArchive : public fileArchive {
 public:
  explicit movieArchive(shotInfoPtr &shot_info_ptr);
  bool makeMovie(const dpath &imageFolder);
  bool convertMovie(const dpath &moviePath);
  bool update(const dpathList &filelist) override;

 protected:
  void insertDB() override;
  void _generateFilePath() override;

 private:
  shotInfoPtr p_info_ptr_;
 private:
  static dstring findFFmpeg() ;
};

CORE_NAMESPACE_E