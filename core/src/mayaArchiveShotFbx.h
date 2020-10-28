//
// Created by teXiao on 2020/10/26.
//

#pragma once
#include "core_global.h"
#include "fileArchive.h"

CORE_NAMESPACE_S
class CORE_EXPORT mayaArchiveShotFbx : public fileArchive {
 public:

  explicit mayaArchiveShotFbx(shotInfoPtr &shot_info_ptr);
  bool exportFbx();
  bool update() override;
  std::map<QString,QString> getInfo();
 protected:
  void insertDB() override;
  void _generateFilePath() override;
 private:
  //数据库文件类
  shotInfoPtr p_info_ptr_;
  mayaArchivePtr p_maya_archive_ptr_;
  //临时导出脚本的地方
  std::shared_ptr<QTemporaryFile> p_temporary_file_;

 private:
  bool readExportJson(const QString &exportPath);
};

CORE_NAMESPACE_E
