/*
 * @Author: your name
 * @Date: 2020-09-27 14:33:42
 * @LastEditTime: 2020-11-26 17:58:06
 * @LastEditors: your name
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\core\src\ueArchive.h
 */
#pragma once

#include <corelib/core_global.h>
#include <corelib/fileArchive/fileArchive.h>

DOODLE_NAMESPACE_S
class CORE_API ueArchive : public fileArchive {
 public:
  explicit ueArchive(fileSqlInfoPtr data);

 protected:
  void insertDB() override;
  void imp_generateFilePath() override;

  void imp_updata(const dpathList &pathList) override;
  void imp_down(const dpath &localPath) override;

 protected:
  fileSqlInfoPtr p_info_;

};

DOODLE_NAMESPACE_E