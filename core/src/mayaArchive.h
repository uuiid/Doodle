#pragma once

#include "core_global.h"

#include "fileArchive.h"

CORE_NAMESPACE_S
class CORE_EXPORT mayaArchive : public fileArchive {
 public:
  explicit mayaArchive(shotInfoPtr & shot_data);


 protected:
  void insertDB() override;
  void _generateFilePath() override;
 private:
  shotInfoPtr p_info_ptr_;

};


CORE_NAMESPACE_E