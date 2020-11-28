/*
 * @Author: your name
 * @Date: 2020-09-27 14:33:42
 * @LastEditTime: 2020-11-26 17:58:06
 * @LastEditors: your name
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\core\src\ueArchive.h
 */
#pragma once

#include "core_global.h"
#include "src/fileArchive/fileArchive.h"

CORE_NAMESPACE_S
class CORE_API ueArchive : public fileArchive {
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