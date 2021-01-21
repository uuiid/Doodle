#pragma once

#include <core_global.h>

DOODLE_NAMESPACE_S
class CommentInfo {
 public:
  //禁止隐式转换
  explicit CommentInfo(fileSqlInfo *file);
  //在取出数据库的路径时使用的方法
  std::vector<std::string> Info(const std::string &pathStr);

  // 正常状况下使用这个
  std::vector<std::string> Info() const;
  void setInfo(const std::string &value);

  void write();
  // 上传数据库是需要的信息
  std::string DBInfo() const;

  void setFileSql(fileSqlInfo *file);

 private:
  fileSqlInfo *p_file_Archive;
  dpathPtr p_path;
  dstringList p_info_list;
};

DOODLE_NAMESPACE_E