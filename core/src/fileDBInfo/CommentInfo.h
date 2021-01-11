#pragma once

#include <core_global.h>
#include <boost/filesystem.hpp>

DOODLE_NAMESPACE_S
class CommentInfo {
 public:
  //禁止隐式转换
  explicit CommentInfo();

  std::vector<std::string> Info(const std::string &pathStr);
  void write(const std::string &Info_value);
  void write(const std::string &Info_value, const dpath &file_Path);

 private:
  dpath p_path_row;
};

DOODLE_NAMESPACE_E