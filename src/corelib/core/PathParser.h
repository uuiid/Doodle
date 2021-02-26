#pragma once
#include <corelib/core_global.h>

#include <rttr/type.h>

#include <boost/signals2.hpp>
namespace doodle {
class Project;
class CoreData;
}  // namespace doodle

namespace doodle::pathParser {
//路径解析器每个都可以有多个
class PathParser {
 private:
  int64_t p_id;
  std::string p_class;    //反射类
  std::regex p_regex;     //解析根路径所需要的正则表达式
  int64_t p_prefix;  //这个是根目录对应的前缀

  std::shared_ptr<PathParser> p_child_parser;
  
  friend class Project;

 public:
  PathParser();
  PathParser(std::string cls, std::string regex);

  DOODLE_DISABLE_COPY(PathParser)

  const int64_t& ID() const noexcept;

  boost::signals2::signal<std::shared_ptr<CoreData>()> getClassInstance;
  std::vector<std::shared_ptr<CoreData>> parser(const fileSys::path& root);
};

}  // namespace doodle::pathParser