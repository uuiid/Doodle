//
// Created by teXiao on 2020/10/26.
//

#pragma once
#include <corelib/core_global.h>
#include <corelib/fileArchive/fileArchive.h>
#include <map>
DOODLE_NAMESPACE_S
class CORE_API mayaArchiveShotFbx : public fileArchive {
 public:
  explicit mayaArchiveShotFbx(shotInfoPtr &shot_info_ptr);
  bool exportFbx(const fileSys::path &shot_data);
  bool update(const fileSys::path &shot_data) override;

 protected:
  void insertDB() override;
  void imp_generateFilePath() override;

 private:
  //数据库文件类
  shotInfoPtr p_info_ptr_;
  //临时导出脚本的地方
  std::shared_ptr<fileSys::path> p_temporary_file_;

 private:
  bool readExportJson(const fileSys::path &exportPath);
};

DOODLE_NAMESPACE_E
