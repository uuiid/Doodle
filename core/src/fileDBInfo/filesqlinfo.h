/*
 * @Author: your name
 * @Date: 2020-09-10 09:56:04
 * @LastEditTime: 2020-12-14 13:31:45
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\core\src\filesqlinfo.h
 */
#pragma once

#include "core_global.h"
#include "src/core/coresqldata.h"

CORE_NAMESPACE_S

class CORE_API fileSqlInfo : public coresqldata {
 public:
  //属性设置和查询
  fileSqlInfo();
  [[nodiscard]] dpathList getFileList() const;
  [[nodiscard]] dpathList getFileList();
  virtual void setFileList(const dpathList &filelist);
  virtual void setFileList(const dstringList &filelist);

  [[nodiscard]] int64_t getVersionP() const;
  void setVersionP(const int64_t value);

  [[nodiscard]] dstringList getInfoP() const;
  void setInfoP(const dstring &value);

  [[nodiscard]] dstring getFileStateP() const;
  void setFileStateP(const dstring &value);

  [[nodiscard]] dstring getSuffixes() const;
  [[nodiscard]] QString getSuffixesQ() const;

  [[nodiscard]] dstring getUser() const;
  [[nodiscard]] QString getUserQ() const;
  //生成路径(只有路径)
  virtual dpath generatePath(const std::string &programFodler) = 0;
  //生成路径带文件名称
  virtual dpath generatePath(const dstring &programFolder,
                             const dstring &suffixes) = 0;
  //生成路径在文件名称中添加前缀
  virtual dpath generatePath(const dstring &programFolder,
                             const dstring &suffixes,
                             const dstring &prefix) = 0;
  //生成文件名称
  virtual dstring generateFileName(const dstring &suffixes) = 0;
  virtual dstring generateFileName(const dstring &suffixes,
                                   const dstring &prefix)   = 0;
  //删除条目
  virtual void insert() override;
  virtual void updateSQL() override;
  virtual void deleteSQL() override;

  virtual bool exist(bool refresh);
  //寻找到和自身一样约束的数据库条目
  virtual dataInfoPtr findSimilar() = 0;

 protected:
  //属性包装

  std::string fileP;
  std::string fileSuffixesP;
  std::string userP;
  int versionP;
  std::string filepathP;
  dstringList infoP;
  std::string fileStateP;
  bool p_b_exist;

 private:
  //解析器包装
  pathParsingPtr p_parser;
  dpathList p_pathlist;

 protected:
  void parsepath(const std::string &pathstr);
  [[nodiscard]] dstringList json_to_strList(const dstring &json_str) const;
  [[nodiscard]] dstring strList_tojson(const dstringList &str_list) const;

  RTTR_ENABLE(coresqldata);
};
inline QString fileSqlInfo::getUserQ() const {
  return QString::fromStdString(getUser());
}
inline QString fileSqlInfo::getSuffixesQ() const {
  return QString::fromStdString(getSuffixes());
}
CORE_NAMESPACE_E
