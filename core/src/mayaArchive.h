#pragma once

#include "core_global.h"

#include "fileArchive.h"

CORE_NAMESPACE_S
class CORE_API mayaArchive : public fileArchive {
 public:
  explicit mayaArchive(fileSqlInfoPtr shot_data);


 protected:
  void insertDB() override;
  void _generateFilePath() override;
 private:
  fileSqlInfoPtr p_info_ptr_;

};


CORE_NAMESPACE_E