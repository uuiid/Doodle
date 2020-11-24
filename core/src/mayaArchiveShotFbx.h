//
// Created by teXiao on 2020/10/26.
//

#pragma once
#include "core_global.h"
#include "fileArchive.h"
#include <map>
CORE_NAMESPACE_S
class CORE_API mayaArchiveShotFbx : public fileArchive {
 public:

  explicit mayaArchiveShotFbx(shotInfoPtr &shot_info_ptr);
  bool exportFbx(const dpath  &shot_data);
  bool update(const dpath &shot_data) override;

 protected:
  void insertDB() override;
  void _generateFilePath() override;
 private:
  //数据库文件类
  shotInfoPtr p_info_ptr_;
  //临时导出脚本的地方
  std::shared_ptr<dpath> p_temporary_file_;

 private:
  bool readExportJson(const dpath &exportPath);
};

CORE_NAMESPACE_E
