#pragma once

#include <core_global.h>
#include <boost/filesystem.hpp>
DOODLE_NAMESPACE_S

class CORE_API pathParsing {
 public:
  //禁止隐式转换
  explicit pathParsing(fileSqlInfo* file);

  // 数据库使用方案
  dpathList Path(const std::string& pathstr);

  // 正常情况下是这个
  dpathList Path() const;
  void setPath(const dpathList& path);

  void write();

  //上传到数据库时，使用这个
  std::string DBInfo() const;

 private:
  fileSqlInfo* p_file_Archive;
  dpathPtr p_path_row;
  dpathList p_path_list;
};

DOODLE_NAMESPACE_E