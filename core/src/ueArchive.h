#pragma once

#include "core_global.h"

#include "fileArchive.h"

CORE_NAMESPACE_S
class CORE_API ueArchive : public fileArchive{
 public:
  explicit ueArchive(fileSqlInfoPtr data);

 protected:
  void insertDB() override;
  void _generateFilePath() override;

  void _updata(const dpathList &pathList) override;
  void _down(const dpath &localPath) override;
 protected:
  fileSqlInfoPtr p_info_;
 private:
  freeSynWrapPtr p_syn;
};

CORE_NAMESPACE_E