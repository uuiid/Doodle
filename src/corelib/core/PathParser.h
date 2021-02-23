#pragma once
#include <corelib/core_global.h>

#include <rttr/type.h>

namespace doodle {
class Project;

}

namespace doodle::pathParser {
//路径解析器每个都可以有多个
class PathParser {
 private:
  std::string p_class_root;  //根目录
  std::string p_class;       //反射类
  std::regex p_regex;        //解析根路径所需要的正则表达式
  std::string p_prefix;      //这个是根目录对应的前缀
  std::string p_suffix;      //这个是根目录对应的后缀

  friend class Project;

 public:
  PathParser();
  DOODLE_DISABLE_COPY(PathParser)

  std::vector<std::reference_wrapper<rttr::variant>> parser(const fileSys::path& root);
};

}  // namespace doodle::pathParser