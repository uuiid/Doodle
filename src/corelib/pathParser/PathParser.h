#pragma once
#include <corelib/core_global.h>

namespace doodle::pathParser {
//路径解析器每个都可以有多个
class PathParser {
 private:
  fileSys::path p_path_root;  //多个根目录
  std::regex p_regex;         //解析根路径所需要的正则表达式
  std::string p_prefix;       //这个是根目录对应的前缀
  std::string p_suffix;       //这个是根目录对应的后缀
 public:
  PathParser();
};

PathParser::PathParser() {
}

}  // namespace doodle::pathParser