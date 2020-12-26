#pragma once

#include <core_global.h>
#include <boost/filesystem.hpp>
CORE_NAMESPACE_S

class CORE_API pathParsing {
 public:
  //禁止隐式转换
  explicit pathParsing();
  ~pathParsing() = default;

  // dpathList getPath();
  dpathList getPath(const std::string& pathstr);
  void write(const dpathList& path_value);
  void write(const dpathList& path_value, const dpath filePath);
  dpathList operator()();

 private:
  dpath p_path_row;
};

CORE_NAMESPACE_E